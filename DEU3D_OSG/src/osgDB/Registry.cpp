/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/

#include <stdio.h>
#include <string.h>

#include <osg/Notify>
#include <osg/Object>
#include <osg/Image>
#include <osg/Shader>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Version>
#include <osg/Timer>

#include <osgDB/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/fstream>
#include <osgDB/Archive>

#include <algorithm>
#include <set>

#include <stdlib.h>

#if defined(__sgi)
    #include <ctype.h>
#elif defined(__GNUC__) || !defined(WIN32) || defined(__MWERKS__)
    #include <cctype>
    using std::tolower;
#endif

#ifdef OSG_LIBRARY_POSTFIX
    #define OSG_LIBRARY_POSTFIX_WITH_QUOTES ADDQUOTES(OSG_LIBRARY_POSTFIX)
#else
    #define OSG_LIBRARY_POSTFIX_WITH_QUOTES ""
#endif

using namespace osg;
using namespace osgDB;


class Creator
{
public:
    Creator(void)
    {
        Registry::instance();
    }
}__creator;

// from MimeTypes.cpp
extern const char* builtinMimeTypeExtMappings[];


class Registry::AvailableReaderWriterIterator
{
public:
    AvailableReaderWriterIterator(Registry::ReaderWriterList& rwList, OpenThreads::ReentrantMutex& pluginMutex):
        _rwList(rwList),
        _pluginMutex(pluginMutex) {}


    ReaderWriter& operator * () { return *get(); }
    ReaderWriter* operator -> () { return get(); }
    
    bool valid() { return get()!=0; }
    
    void operator ++() 
    {
        _rwUsed.insert(get());
    }
    

protected:

    AvailableReaderWriterIterator& operator = (const AvailableReaderWriterIterator&) { return *this; }

    Registry::ReaderWriterList&     _rwList;
    OpenThreads::ReentrantMutex&    _pluginMutex;
    
    std::set<ReaderWriter*>         _rwUsed;

    ReaderWriter* get() 
    {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
        Registry::ReaderWriterList::iterator itr=_rwList.begin();
        for(;itr!=_rwList.end();++itr)
        {
            if (_rwUsed.find(itr->get())==_rwUsed.end())
            {
                return itr->get();
            }
        }
        return 0;
    }

};

class Registry::AvailableArchiveIterator
{
public:
    AvailableArchiveIterator(Registry::ArchiveCache& archives, OpenThreads::ReentrantMutex& mutex):
        _archives(archives),
        _mutex(mutex) {}


    Archive& operator * () { return *get(); }
    Archive* operator -> () { return get(); }

    bool valid() { return get()!=0; }

    void operator ++()
    {
        _archivesUsed.insert(get());
    }


protected:

    AvailableArchiveIterator& operator = (const AvailableArchiveIterator&) { return *this; }

    Registry::ArchiveCache&         _archives;
    OpenThreads::ReentrantMutex&    _mutex;

    std::set<Archive*>              _archivesUsed;

    Archive* get()
    {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_mutex);
        Registry::ArchiveCache::iterator itr=_archives.begin();
        for(;itr!=_archives.end();++itr)
        {
            if (_archivesUsed.find(itr->second.get())==_archivesUsed.end())
            {
                return itr->second.get();
            }
        }
        return 0;
    }

};

void PrintFilePathList(std::ostream& stream,const FilePathList& filepath)
{
    for(FilePathList::const_iterator itr=filepath.begin();
        itr!=filepath.end();
        ++itr)
    {
        stream << "    "<< *itr<<std::endl;
    }
}

Registry* Registry::instance(bool erase)
{
    static ref_ptr<Registry> s_registry = new Registry;
    if (erase) 
    {   
        s_registry->destruct();
        s_registry = 0;
    }
    return s_registry.get(); // will return NULL on erase
}


// definition of the Registry
Registry::Registry()
{
    // comment out because it was causing problems under OSX - causing it to crash osgconv when constructing ostream in osg::notify().
    // OSG_INFO << "Constructing osg::Registry"<<std::endl;

    _buildKdTreesHint = Options::NO_PREFERENCE;
    _kdTreeBuilder = new osg::KdTreeBuilder;

    _expiryDelay = 10.0;

    _createNodeFromImage = false;
    _openingLibrary = false;

    // add default osga archive extension
    _archiveExtList.push_back("osga");
    
    initFilePathLists();


#if defined(DARWIN_QUICKTIME)

    addFileExtensionAlias("jpg",  "qt");
    addFileExtensionAlias("jpe",  "qt");
    addFileExtensionAlias("jpeg", "qt");
    addFileExtensionAlias("tif",  "qt");
    addFileExtensionAlias("tiff", "qt");
    addFileExtensionAlias("gif",  "qt");
    addFileExtensionAlias("png",  "qt");
    addFileExtensionAlias("psd",  "qt");
    addFileExtensionAlias("tga",  "qt");
    addFileExtensionAlias("flv",  "qt");
    addFileExtensionAlias("dv",   "qt");
    #if !defined(DARWIN_QTKIT)

        addFileExtensionAlias("mov",  "qt");
        addFileExtensionAlias("avi",  "qt");
        addFileExtensionAlias("mpg",  "qt");
        addFileExtensionAlias("mpv",  "qt");
        addFileExtensionAlias("mp4",  "qt");
        addFileExtensionAlias("m4v",  "qt");
        addFileExtensionAlias("3gp",  "qt");
        // Add QuickTime live support for OSX
        addFileExtensionAlias("live", "qt");
    #endif
#else
    addFileExtensionAlias("jpg",  "jpeg");
    addFileExtensionAlias("jpe",  "jpeg");
    addFileExtensionAlias("tif",  "tiff");

    // really need to decide this at runtime...
    #if defined(USE_XINE)

        addFileExtensionAlias("mov",  "xine");
        addFileExtensionAlias("mpg",  "xine");
        addFileExtensionAlias("ogv",  "xine");
        addFileExtensionAlias("mpv",  "xine");
        addFileExtensionAlias("dv",   "xine");
        addFileExtensionAlias("avi",  "xine");
        addFileExtensionAlias("wmv",  "xine");
        addFileExtensionAlias("flv",  "xine");
    #endif

    // support QuickTime for Windows
    // Logic error here. It is possible for Apple to not define Quicktime and end up in
    // this Quicktime for Windows block. So add an extra check to avoid QTKit clashes.
    #if defined(USE_QUICKTIME) && !defined(DARWIN_QTKIT)

        addFileExtensionAlias("mov",  "qt");
        addFileExtensionAlias("live", "qt");
        addFileExtensionAlias("mpg",  "qt");
        addFileExtensionAlias("avi",  "qt");
    #endif
#endif

    // add alias for the text/freetype plugin.
    addFileExtensionAlias("ttf",    "freetype");  // true type
    addFileExtensionAlias("ttc",    "freetype");  // true type
    addFileExtensionAlias("cid",    "freetype");  // Postscript CID-Fonts
    addFileExtensionAlias("cff",    "freetype");  // OpenType
    addFileExtensionAlias("cef",    "freetype");  // OpenType
    addFileExtensionAlias("fon",    "freetype");  // Windows bitmap fonts
    addFileExtensionAlias("fnt",    "freetype");  // Windows bitmap fonts
    addFileExtensionAlias("text3d", "freetype"); // use 3D Font instead of 2D Font

    // wont't add type1 and type2 until resolve extension collision with Performer binary and ascii files.
    // addFileExtensionAlias("pfb",   "freetype");  // type1 binary
    // addFileExtensionAlias("pfa",   "freetype");  // type2 ascii

    // add built-in mime-type extension mappings
    for( int i=0; ; i+=2 )
    {
        std::string mimeType = builtinMimeTypeExtMappings[i];
        if ( mimeType.length() == 0 )
            break;
        addMimeTypeExtensionMapping( mimeType, builtinMimeTypeExtMappings[i+1] );
    }

    _objectWrapperManager = new ObjectWrapperManager;
    _deprecatedDotOsgWrapperManager = new DeprecatedDotOsgWrapperManager;
}


Registry::~Registry()
{
    destruct();
}

void Registry::destruct()
{
    // OSG_NOTICE<<"Registry::destruct()"<<std::endl;

    // clean up the SharedStateManager 
    _sharedStateManager = 0;

    // object cache clear needed here to prevent crash in unref() of
    // the objects it contains when running the TXP plugin.
    // Not sure why, but perhaps there is is something in a TXP plugin
    // which deletes the data before its ref count hits zero, perhaps
    // even some issue with objects be allocated by a plugin that is
    // maintained after that plugin is deleted...  Robert Osfield, Jan 2004.
    clearObjectCache();
    clearArchiveCache();
    

    // unload all the plugin before we finally destruct.
    closeAllLibraries();
}

#include <iostream>

void Registry::initDataFilePathList()
{
    FilePathList filepath;
    //
    // set up data file paths
    //
    char *ptr;
  
    if( (ptr = getenv( "OSG_FILE_PATH" )) )
    {
        //OSG_NOTIFY(DEBUG_INFO) << "OSG_FILE_PATH("<<ptr<<")"<<std::endl;
        convertStringPathIntoFilePathList(ptr, filepath);
    }
    else if( (ptr = getenv( "OSGFILEPATH" )) )
    {
        //OSG_NOTIFY(DEBUG_INFO) << "OSGFILEPATH("<<ptr<<")"<<std::endl;
        convertStringPathIntoFilePathList(ptr, filepath);
    }

    osgDB::appendPlatformSpecificResourceFilePaths(filepath);
    setDataFilePathList(filepath);
    
}

void Registry::setDataFilePathList(const std::string& paths)
{
    _dataFilePath.clear(); 
    convertStringPathIntoFilePathList(paths,_dataFilePath);
}

void Registry::setLibraryFilePathList(const std::string& paths) { _libraryFilePath.clear(); convertStringPathIntoFilePathList(paths,_libraryFilePath); }



void Registry::initLibraryFilePathList()
{
    //
    // set up library paths
    //
    char* ptr;
    if( (ptr = getenv( "OSG_LIBRARY_PATH")) )
    {
        //OSG_NOTIFY(DEBUG_INFO) << "OSG_LIBRARY_PATH("<<ptr<<")"<<std::endl;
        setLibraryFilePathList(ptr);
    }
    else if( (ptr = getenv( "OSG_LD_LIBRARY_PATH")) )
    {
        //OSG_NOTIFY(DEBUG_INFO) << "OSG_LD_LIBRARY_PATH("<<ptr<<")"<<std::endl;
        setLibraryFilePathList(ptr);
    }
    
    appendPlatformSpecificLibraryFilePaths(_libraryFilePath);

}


void Registry::addReaderWriter(ReaderWriter* rw)
{
    if (rw==0L) return;

    // OSG_NOTIFY(INFO) << "osg::Registry::addReaderWriter("<<rw->className()<<")"<< std::endl;

    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);

    _rwList.push_back(rw);

}


void Registry::removeReaderWriter(ReaderWriter* rw)
{
    if (rw==0L) return;

//    OSG_NOTIFY(INFO) << "osg::Registry::removeReaderWriter();"<< std::endl;

    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);

    ReaderWriterList::iterator rwitr = std::find(_rwList.begin(),_rwList.end(),rw);
    if (rwitr!=_rwList.end())
    {
        _rwList.erase(rwitr);
    }

}

ImageProcessor* Registry::getImageProcessor()
{
    {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
        if (!_ipList.empty())
        {
            return _ipList.front().get();
        }
    }
    return getImageProcessorForExtension("nvtt");
}

ImageProcessor* Registry::getImageProcessorForExtension(const std::string& ext)
{
    {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
        if (!_ipList.empty())
        {
            return _ipList.front().get();
        }
    }

    std::string libraryName = createLibraryNameForExtension(ext);
    OSG_NOTICE << "Now checking for plug-in "<<libraryName<< std::endl;
    if (loadLibrary(libraryName)==LOADED)
    {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
        if (!_ipList.empty())
        {
            OSG_NOTICE << "Loaded plug-in "<<libraryName<<" and located ImageProcessor"<< std::endl;
            return _ipList.front().get();
        }
    }
    return 0;
}

void Registry::addImageProcessor(ImageProcessor* ip)
{
    if (ip==0L) return;

    OSG_NOTIFY(NOTICE) << "osg::Registry::addImageProcessor("<<ip->className()<<")"<< std::endl;

    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);

    _ipList.push_back(ip);

}


void Registry::removeImageProcessor(ImageProcessor* ip)
{
    if (ip==0L) return;

    OSG_NOTIFY(NOTICE) << "osg::Registry::removeImageProcessor();"<< std::endl;

    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);

    ImageProcessorList::iterator ipitr = std::find(_ipList.begin(),_ipList.end(),ip);
    if (ipitr!=_ipList.end())
    {
        _ipList.erase(ipitr);
    }

}


void Registry::addFileExtensionAlias(const std::string mapExt, const std::string toExt)
{
    _extAliasMap[mapExt] = toExt;
}

void Registry::addMimeTypeExtensionMapping(const std::string fromMimeType, const std::string toExt)
{
    _mimeTypeExtMap[fromMimeType] = toExt;
}

bool Registry::readPluginAliasConfigurationFile( const std::string& file )
{
    std::string fileName = osgDB::findDataFile( file );
    if (fileName.empty())
    {
        OSG_NOTIFY( osg::WARN) << "Can't find plugin alias config file \"" << file << "\"." << std::endl;
        return false;
    }

    osgDB::ifstream ifs;
    ifs.open( fileName.c_str() );
    if (!ifs.good())
    {
        OSG_NOTIFY( osg::WARN) << "Can't open plugin alias config file \"" << fileName << "\"." << std::endl;
        return false;
    }

    int lineNum( 0 );
    while (ifs.good())
    {
        std::string raw;
        ++lineNum;
        std::getline( ifs, raw );
        std::string ln = trim( raw );
        if (ln.empty()) continue;
        if (ln[0] == '#') continue;

        std::string::size_type spIdx = ln.find_first_of( " \t" );
        if (spIdx == ln.npos)
        {
            // mapExt and toExt must be on the same line, separated by a space.
            OSG_NOTIFY( osg::WARN) << file << ", line " << lineNum << ": Syntax error: missing space in \"" << raw << "\"." << std::endl;
            continue;
        }

        const std::string mapExt = trim( ln.substr( 0, spIdx ) );
        const std::string toExt = trim( ln.substr( spIdx+1 ) );
        addFileExtensionAlias( mapExt, toExt );
    }
    return true;
}

std::string Registry::trim( const std::string& str )
{
    if (!str.size()) return str;
    std::string::size_type first = str.find_first_not_of( " \t" );
    std::string::size_type last = str.find_last_not_of( "  \t\r\n" );
    if ((first==str.npos) || (last==str.npos)) return std::string( "" );
    return str.substr( first, last-first+1 );
}


std::string Registry::createLibraryNameForFile(const std::string& fileName)
{
    return createLibraryNameForExtension(getFileExtension(fileName));
}

std::string Registry::createLibraryNameForExtension(const std::string& ext)
{
    std::string lowercase_ext;
    for(std::string::const_iterator sitr=ext.begin();
        sitr!=ext.end();
        ++sitr)
    {
        lowercase_ext.push_back(tolower(*sitr));
    }

    ExtensionAliasMap::iterator itr=_extAliasMap.find(lowercase_ext);
    if (itr!=_extAliasMap.end() && ext != itr->second) return createLibraryNameForExtension(itr->second);

#if defined(__CYGWIN__)
    return "cygwin_"+"enginedb_"+lowercase_ext+OSG_LIBRARY_POSTFIX_WITH_QUOTES+".dll";
#elif defined(__MINGW32__)
    return "mingw_"+"enginedb_"+lowercase_ext+OSG_LIBRARY_POSTFIX_WITH_QUOTES+".dll";
#elif defined(WIN32)
    return "enginedb_"+lowercase_ext+OSG_LIBRARY_POSTFIX_WITH_QUOTES+".dll";
#elif macintosh
    return "enginedb_"+lowercase_ext+OSG_LIBRARY_POSTFIX_WITH_QUOTES;
#else
    return "enginedb_"+lowercase_ext+OSG_LIBRARY_POSTFIX_WITH_QUOTES+ADDQUOTES(OSG_PLUGIN_EXTENSION);
#endif

}

std::string Registry::createLibraryNameForNodeKit(const std::string& name)
{
#if defined(__CYGWIN__)
    return "cyg"+name+OSG_LIBRARY_POSTFIX_WITH_QUOTES+".dll";
#elif defined(__MINGW32__)
    return "lib"+name+OSG_LIBRARY_POSTFIX_WITH_QUOTES+".dll";
#elif defined(WIN32)
    return name+OSG_LIBRARY_POSTFIX_WITH_QUOTES+".dll";
#elif macintosh
    return name+OSG_LIBRARY_POSTFIX_WITH_QUOTES;
#else
    return "lib"+name+OSG_LIBRARY_POSTFIX_WITH_QUOTES + ADDQUOTES(OSG_PLUGIN_EXTENSION);
#endif
}

Registry::LoadStatus Registry::loadLibrary(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);

    DynamicLibraryList::iterator ditr = getLibraryItr(fileName);
    if (ditr!=_dlList.end()) return PREVIOUSLY_LOADED;

    _openingLibrary=true;

    DynamicLibrary* dl = DynamicLibrary::loadLibrary(fileName);
    _openingLibrary=false;

    if (dl)
    {
        _dlList.push_back(dl);
        return LOADED;
    }
    return NOT_LOADED;
}


bool Registry::closeLibrary(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
    DynamicLibraryList::iterator ditr = getLibraryItr(fileName);
    if (ditr!=_dlList.end())
    {
        _dlList.erase(ditr);
        return true;
    }
    return false;
}

void Registry::closeAllLibraries()
{
    // OSG_NOTICE<<"Registry::closeAllLibraries()"<<std::endl;
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
    _dlList.clear();
}

Registry::DynamicLibraryList::iterator Registry::getLibraryItr(const std::string& fileName)
{
    DynamicLibraryList::iterator ditr = _dlList.begin();
    for(;ditr!=_dlList.end();++ditr)
    {
        if ((*ditr)->getName()==fileName) return ditr;
    }
    return _dlList.end();
}

DynamicLibrary* Registry::getLibrary(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);
    DynamicLibraryList::iterator ditr = getLibraryItr(fileName);
    if (ditr!=_dlList.end()) return ditr->get();
    else return NULL;
}

ReaderWriter* Registry::getReaderWriterForExtension(const std::string& ext)
{
    // record the existing reader writer.
    std::set<ReaderWriter*> rwOriginal;

    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(_pluginMutex);

    // first attemt one of the installed loaders
    for(ReaderWriterList::iterator itr=_rwList.begin();
        itr!=_rwList.end();
        ++itr)
    {
        rwOriginal.insert(itr->get());
        if((*itr)->acceptsExtension(ext)) return (*itr).get();
    }
    
    // now look for a plug-in to load the file.
    std::string libraryName = createLibraryNameForExtension(ext);
    OSG_NOTIFY(INFO) << "Now checking for plug-in "<<libraryName<< std::endl;
    if (loadLibrary(libraryName)==LOADED)
    {
        for(ReaderWriterList::iterator itr=_rwList.begin();
            itr!=_rwList.end();
            ++itr)
        {
            if (rwOriginal.find(itr->get())==rwOriginal.end())
            {
                if((*itr)->acceptsExtension(ext)) return (*itr).get();
            }
        }
    }

    return NULL;

}

ReaderWriter* Registry::getReaderWriterForMimeType(const std::string& mimeType)
{
    MimeTypeExtensionMap::const_iterator i = _mimeTypeExtMap.find( mimeType );
    return i != _mimeTypeExtMap.end()?
        getReaderWriterForExtension( i->second ) :
        NULL;
}

#if 0
struct concrete_wrapper: basic_type_wrapper 
{
    virtual ~concrete_wrapper() {}
    concrete_wrapper(const osg::Object *myobj) : myobj_(myobj) {}
    bool matches(const osg::Object *proto) const
    {
        return myobj_->isSameKindAs(proto);
    }
    const osg::Object *myobj_;
};
#endif



struct Registry::ReadObjectFunctor : public Registry::ReadFunctor
{
    ReadObjectFunctor(const std::string& filename, const Options* options):ReadFunctor(filename,options) {}
    ReadObjectFunctor(const ID& id, const Options* options):ReadFunctor(id,options) {}

    virtual ReaderWriter::ReadResult doRead(ReaderWriter& rw) const
    {
        if(_id.isValid())
            return rw.readObject(_id, _options);
        else
            return rw.readObject(_filename, _options);
    }
    virtual bool isValid(ReaderWriter::ReadResult& readResult) const { return readResult.validObject(); }
    virtual bool isValid(osg::Object* object) const { return object!=0;  }
};

struct Registry::ReadImageFunctor : public Registry::ReadFunctor
{
    ReadImageFunctor(const std::string& filename, const Options* options):ReadFunctor(filename,options) {}
    ReadImageFunctor(const ID& id, const Options* options):ReadFunctor(id,options) {}

    virtual ReaderWriter::ReadResult doRead(ReaderWriter& rw)const
    {
        if(_id.isValid())
            return rw.readImage(_id, _options);
        else
            return rw.readImage(_filename, _options);
    }
    virtual bool isValid(ReaderWriter::ReadResult& readResult) const { return readResult.validImage(); }
    virtual bool isValid(osg::Object* object) const { return dynamic_cast<osg::Image*>(object)!=0;  }
};

struct Registry::ReadHeightFieldFunctor : public Registry::ReadFunctor
{
    ReadHeightFieldFunctor(const std::string& filename, const Options* options):ReadFunctor(filename,options) {}
    ReadHeightFieldFunctor(const ID& id, const Options* options):ReadFunctor(id,options) {}

    virtual ReaderWriter::ReadResult doRead(ReaderWriter& rw) const
    {
        if(_id.isValid())
            return rw.readHeightField(_id, _options);
        else
            return rw.readHeightField(_filename, _options);
    }
    virtual bool isValid(ReaderWriter::ReadResult& readResult) const { return readResult.validHeightField(); }
    virtual bool isValid(osg::Object* object) const { return dynamic_cast<osg::HeightField*>(object)!=0;  }
};

struct Registry::ReadNodeFunctor : public Registry::ReadFunctor
{
    ReadNodeFunctor(const std::string& filename, const Options* options):ReadFunctor(filename,options) {}
    ReadNodeFunctor(const ID& id, const Options* options):ReadFunctor(_id,options) {}

    virtual ReaderWriter::ReadResult doRead(ReaderWriter& rw) const
    {
        if(_id.isValid())
            return rw.readNode(_id, _options);
        else
            return rw.readNode(_filename, _options);
    }
    virtual bool isValid(ReaderWriter::ReadResult& readResult) const { return readResult.validNode(); }
    virtual bool isValid(osg::Object* object) const { return dynamic_cast<osg::Node*>(object)!=0;  }

};

struct Registry::ReadArchiveFunctor : public Registry::ReadFunctor
{
    ReadArchiveFunctor(const std::string& filename, ReaderWriter::ArchiveStatus status, unsigned int indexBlockSizeHint, const Options* options):
        ReadFunctor(filename,options),
        _status(status),
        _indexBlockSizeHint(indexBlockSizeHint) {}
        
    ReaderWriter::ArchiveStatus _status;
    unsigned int _indexBlockSizeHint;

    virtual ReaderWriter::ReadResult doRead(ReaderWriter& rw) const { return rw.openArchive(_filename, _status, _indexBlockSizeHint, _options); }
    virtual bool isValid(ReaderWriter::ReadResult& readResult) const { return readResult.validArchive(); }
    virtual bool isValid(osg::Object* object) const { return dynamic_cast<osgDB::Archive*>(object)!=0;  }

};

struct Registry::ReadShaderFunctor : public Registry::ReadFunctor
{
    ReadShaderFunctor(const std::string& filename, const Options* options):ReadFunctor(filename,options) {}
    ReadShaderFunctor(const ID& id, const Options* options):ReadFunctor(id,options) {}

    virtual ReaderWriter::ReadResult doRead(ReaderWriter& rw)const
    {
        if(_id.isValid())
            return rw.readShader(_id, _options);
        else
            return rw.readShader(_filename, _options);
    }
    virtual bool isValid(ReaderWriter::ReadResult& readResult) const { return readResult.validShader(); }
    virtual bool isValid(osg::Object* object) const { return dynamic_cast<osg::Shader*>(object)!=0;  }
};

void Registry::addArchiveExtension(const std::string ext)
{
    for(ArchiveExtensionList::iterator aitr=_archiveExtList.begin();
        aitr!=_archiveExtList.end();
        ++aitr)
    {
        if ( (*aitr) == ext)   // extension already in archive extension list
            return;
    }
    _archiveExtList.push_back(ext);
}

std::string Registry::findDataFileImplementation(const std::string& filename, const Options* options, CaseSensitivity caseSensitivity)
{
    if (filename.empty()) return filename;

    if(fileExists(filename))
    {
        OSG_DEBUG << "FindFileInPath(" << filename << "): returning " << filename << std::endl;
        return filename;
    }

    std::string fileFound;

    if (options && !options->getDatabasePathList().empty())
    {
        fileFound = findFileInPath(filename, options->getDatabasePathList(), caseSensitivity);
        if (!fileFound.empty()) return fileFound;
    }

    const FilePathList& filepath = Registry::instance()->getDataFilePathList();
    if (!filepath.empty())
    {
        fileFound = findFileInPath(filename, filepath,caseSensitivity);
        if (!fileFound.empty()) return fileFound;
    }


    // if a directory is included in the filename, get just the (simple) filename itself and try that
    std::string simpleFileName = getSimpleFileName(filename);
    if (simpleFileName!=filename)
    {

        if(fileExists(simpleFileName))
        {
            OSG_DEBUG << "FindFileInPath(" << filename << "): returning " << filename << std::endl;
            return simpleFileName;
        }

        if (options && !options->getDatabasePathList().empty())
        {
            fileFound = findFileInPath(simpleFileName, options->getDatabasePathList(), caseSensitivity);
            if (!fileFound.empty()) return fileFound;
        }

        if (!filepath.empty())
        {
            fileFound = findFileInPath(simpleFileName, filepath,caseSensitivity);
            if (!fileFound.empty()) return fileFound;
        }

    }

    // return empty string.
    return std::string();
}

std::string Registry::findLibraryFileImplementation(const std::string& filename, const Options* options, CaseSensitivity caseSensitivity)
{
    if (filename.empty())
        return filename;

    const FilePathList& filepath = Registry::instance()->getLibraryFilePathList();

    std::string fileFound = findFileInPath(filename, filepath,caseSensitivity);
    if (!fileFound.empty())
        return fileFound;

    if(fileExists(filename))
    {
        OSG_DEBUG << "FindFileInPath(" << filename << "): returning " << filename << std::endl;
        return filename;
    }

    // if a directory is included in the filename, get just the (simple) filename itself and try that
    std::string simpleFileName = getSimpleFileName(filename);
    if (simpleFileName!=filename)
    {
        std::string fileFound = findFileInPath(simpleFileName, filepath,caseSensitivity);
        if (!fileFound.empty()) return fileFound;
    }

    // failed return empty string.
    return std::string();
}



ReaderWriter::ReadResult Registry::read(const ReadFunctor& readFunctor)
{
    for(ArchiveExtensionList::iterator aitr=_archiveExtList.begin();
        aitr!=_archiveExtList.end();
        ++aitr)
    {
        std::string archiveExtension = "." + (*aitr);

        std::string::size_type positionArchive = readFunctor._filename.find(archiveExtension+'/');
        if (positionArchive==std::string::npos) positionArchive = readFunctor._filename.find(archiveExtension+'\\');
        if (positionArchive!=std::string::npos)
        {
            std::string::size_type endArchive = positionArchive + archiveExtension.length();
            std::string archiveName( readFunctor._filename.substr(0,endArchive));
            std::string fileName(readFunctor._filename.substr(endArchive+1,std::string::npos));
            OSG_INFO<<"Contains archive : "<<readFunctor._filename<<std::endl;
            OSG_INFO<<"         archive : "<<archiveName<<std::endl;
            OSG_INFO<<"         filename : "<<fileName<<std::endl;
        
            ReaderWriter::ReadResult result = openArchiveImplementation(archiveName,ReaderWriter::READ, 4096, readFunctor._options);
        
            if (!result.validArchive()) return result;

            osgDB::Archive* archive = result.getArchive();
        
            //if valid options were passed through the read functor clone them
            //otherwise make new options
            osg::ref_ptr<osgDB::ReaderWriter::Options> options = readFunctor._options ?
                readFunctor._options->cloneOptions() :
                new osgDB::ReaderWriter::Options;

            options->setDatabasePath(archiveName);

            return archive->readObject(fileName,options.get());
        }
    }
    
    // record the errors reported by readerwriters.
    typedef std::vector<ReaderWriter::ReadResult> Results;
    Results results;

    // first attempt to load the file from existing ReaderWriter's
    AvailableReaderWriterIterator itr(_rwList, _pluginMutex);
    for(;itr.valid();++itr)
    {
        ReaderWriter::ReadResult rr = readFunctor.doRead(*itr);
        if (readFunctor.isValid(rr)) return rr;
        else results.push_back(rr);
    }

    // check loaded archives.
    AvailableArchiveIterator aaitr(_archiveCache, _archiveCacheMutex);
    for(;aaitr.valid();++aaitr)
    {
        ReaderWriter::ReadResult rr = readFunctor.doRead(*aaitr);
        if (readFunctor.isValid(rr)) return rr;
        else results.push_back(rr);
    }


    if (!results.empty())
    {
        unsigned int num_FILE_NOT_HANDLED = 0;
        unsigned int num_FILE_NOT_FOUND = 0;
        unsigned int num_ERROR_IN_READING_FILE = 0;

        Results::iterator ritr;
        for(ritr=results.begin();
            ritr!=results.end();
            ++ritr)
        {
            if (ritr->status()==ReaderWriter::ReadResult::FILE_NOT_HANDLED) ++num_FILE_NOT_HANDLED;
            else if (ritr->status()==ReaderWriter::ReadResult::NOT_IMPLEMENTED) ++num_FILE_NOT_HANDLED;//Freetype and others
            else if (ritr->status()==ReaderWriter::ReadResult::FILE_NOT_FOUND) ++num_FILE_NOT_FOUND;
            else if (ritr->status()==ReaderWriter::ReadResult::ERROR_IN_READING_FILE) ++num_ERROR_IN_READING_FILE;
        }
        
        if (num_FILE_NOT_HANDLED!=results.size())
        {
            for(ritr=results.begin(); ritr!=results.end(); ++ritr)
            {
                if (ritr->status()==ReaderWriter::ReadResult::ERROR_IN_READING_FILE)
                {
                    // OSG_NOTICE<<"Warning: error reading file \""<<readFunctor._filename<<"\""<<std::endl;
                    return *ritr;
                }
            }

            for(ritr=results.begin(); ritr!=results.end(); ++ritr)
            {
                if (ritr->status()==ReaderWriter::ReadResult::FILE_NOT_FOUND)
                {
                    //OSG_NOTICE<<"Warning: could not find file \""<<readFunctor._filename<<"\""<<std::endl;
                    return *ritr;
                }
            }
        }
    }

    results.clear();

    // now look for a plug-in to load the file.
    std::string libraryName = createLibraryNameForFile(readFunctor._filename);
    if (loadLibrary(libraryName)!=NOT_LOADED)
    {
        for(;itr.valid();++itr)
        {
            ReaderWriter::ReadResult rr = readFunctor.doRead(*itr);
            if (readFunctor.isValid(rr)) return rr;
            else results.push_back(rr);
        }
    }

    if (!results.empty())
    {
        unsigned int num_FILE_NOT_HANDLED = 0;
        unsigned int num_FILE_NOT_FOUND = 0;
        unsigned int num_ERROR_IN_READING_FILE = 0;

        Results::iterator ritr;
        for(ritr=results.begin();
            ritr!=results.end();
            ++ritr)
        {
            if (ritr->status()==ReaderWriter::ReadResult::FILE_NOT_HANDLED) ++num_FILE_NOT_HANDLED;
            else if (ritr->status()==ReaderWriter::ReadResult::FILE_NOT_FOUND) ++num_FILE_NOT_FOUND;
            else if (ritr->status()==ReaderWriter::ReadResult::ERROR_IN_READING_FILE) ++num_ERROR_IN_READING_FILE;
        }
        
        if (num_FILE_NOT_HANDLED!=results.size())
        {
            for(ritr=results.begin(); ritr!=results.end(); ++ritr)
            {
                if (ritr->status()==ReaderWriter::ReadResult::ERROR_IN_READING_FILE)
                {
                    // OSG_NOTICE<<"Warning: error reading file \""<<readFunctor._filename<<"\""<<std::endl;
                    return *ritr;
                }
            }

            for(ritr=results.begin(); ritr!=results.end(); ++ritr)
            {
                if (ritr->status()==ReaderWriter::ReadResult::FILE_NOT_FOUND)
                {
                    // OSG_NOTICE<<"Warning: could not find file \""<<readFunctor._filename<<"\""<<std::endl;
                    return *ritr;
                }
            }
        }
    }
    else
    {
        return ReaderWriter::ReadResult("Warning: Could not find plugin to read objects from file \""+readFunctor._filename+"\".");
    }

    return results.front();
}

ReaderWriter::ReadResult Registry::readImplementation(const ReadFunctor& readFunctor)
{
    return read(readFunctor);
}


ReaderWriter::ReadResult Registry::openArchiveImplementation(const std::string& fileName, ReaderWriter::ArchiveStatus status, unsigned int indexBlockSizeHint, const Options* options)
{
    osg::ref_ptr<osgDB::Archive> archive = getRefFromArchiveCache(fileName);
    if (archive.valid()) return archive.get();

    ReaderWriter::ReadResult result = readImplementation(ReadArchiveFunctor(fileName, status, indexBlockSizeHint, options));

    return result;
}


ReaderWriter::ReadResult Registry::readObjectImplementation(const std::string& fileName,const Options* options)
{
    return readImplementation(ReadObjectFunctor(fileName, options));
}
ReaderWriter::ReadResult Registry::readObjectImplementation(const ID& id,const Options* options)
{
    return readImplementation(ReadObjectFunctor(id, options));
}

ReaderWriter::WriteResult Registry::writeObjectImplementation(const Object& obj,const std::string& fileName,const Options* options)
{
    // record the errors reported by readerwriters.
    typedef std::vector<ReaderWriter::WriteResult> Results;
    Results results;

    // first attempt to load the file from existing ReaderWriter's
    AvailableReaderWriterIterator itr(_rwList, _pluginMutex);
    for(;itr.valid();++itr)
    {
        ReaderWriter::WriteResult rr = itr->writeObject(obj,fileName,options);
        if (rr.success()) return rr;
        else results.push_back(rr);
    }

    // now look for a plug-in to save the file.
    std::string libraryName = createLibraryNameForFile(fileName);
    if (loadLibrary(libraryName)==LOADED)
    {
        for(;itr.valid();++itr)
        {
            ReaderWriter::WriteResult rr = itr->writeObject(obj,fileName,options);
            if (rr.success()) return rr;
            else results.push_back(rr);
        }
    }

    if (results.empty())
    {
        return ReaderWriter::WriteResult("Warning: Could not find plugin to write objects to file \""+fileName+"\".");
    }

    if (results.front().message().empty())
    {
        switch(results.front().status())
        {
            case(ReaderWriter::WriteResult::FILE_NOT_HANDLED): results.front().message() = "Warning: Write to \""+fileName+"\" not supported."; break;
            case(ReaderWriter::WriteResult::ERROR_IN_WRITING_FILE): results.front().message() = "Warning: Error in writing to \""+fileName+"\"."; break;
            default: break;
        }
    }

    return results.front();
}



ReaderWriter::ReadResult Registry::readImageImplementation(const std::string& fileName,const Options* options)
{
    return readImplementation(ReadImageFunctor(fileName, options));
}
ReaderWriter::ReadResult Registry::readImageImplementation(const ID& id,const Options* options)
{
    return readImplementation(ReadImageFunctor(id, options));
}

ReaderWriter::WriteResult Registry::writeImageImplementation(const Image& image,const std::string& fileName,const Options* options)
{
    // record the errors reported by readerwriters.
    typedef std::vector<ReaderWriter::WriteResult> Results;
    Results results;

    // first attempt to load the file from existing ReaderWriter's
    AvailableReaderWriterIterator itr(_rwList, _pluginMutex);
    for(;itr.valid();++itr)
    {
        ReaderWriter::WriteResult rr = itr->writeImage(image,fileName,options);
        if (rr.success()) return rr;
        else results.push_back(rr);
    }

    results.clear();

    // now look for a plug-in to save the file.
    std::string libraryName = createLibraryNameForFile(fileName);
    if (loadLibrary(libraryName)==LOADED)
    {
        for(;itr.valid();++itr)
        {
            ReaderWriter::WriteResult rr = itr->writeImage(image,fileName,options);
            if (rr.success()) return rr;
            else results.push_back(rr);
        }
    }

    if (results.empty())
    {
        return ReaderWriter::WriteResult("Warning: Could not find plugin to write image to file \""+fileName+"\".");
    }
    
    if (results.front().message().empty())
    {
        switch(results.front().status())
        {
            case(ReaderWriter::WriteResult::FILE_NOT_HANDLED): results.front().message() = "Warning: Write to \""+fileName+"\" not supported."; break;
            case(ReaderWriter::WriteResult::ERROR_IN_WRITING_FILE): results.front().message() = "Warning: Error in writing to \""+fileName+"\"."; break;
            default: break;
        }
    }

    return results.front();
}


ReaderWriter::ReadResult Registry::readHeightFieldImplementation(const std::string& fileName,const Options* options)
{
    return readImplementation(ReadHeightFieldFunctor(fileName, options));
}
ReaderWriter::ReadResult Registry::readHeightFieldImplementation(const ID& id,const Options* options)
{
    return readImplementation(ReadHeightFieldFunctor(id, options));
}

ReaderWriter::WriteResult Registry::writeHeightFieldImplementation(const HeightField& HeightField,const std::string& fileName,const Options* options)
{
    // record the errors reported by readerwriters.
    typedef std::vector<ReaderWriter::WriteResult> Results;
    Results results;

    // first attempt to load the file from existing ReaderWriter's
    AvailableReaderWriterIterator itr(_rwList, _pluginMutex);
    for(;itr.valid();++itr)
    {
        ReaderWriter::WriteResult rr = itr->writeHeightField(HeightField,fileName,options);
        if (rr.success()) return rr;
        else results.push_back(rr);
    }

    results.clear();

    // now look for a plug-in to save the file.
    std::string libraryName = createLibraryNameForFile(fileName);
    if (loadLibrary(libraryName)==LOADED)
    {
        for(;itr.valid();++itr)
        {
            ReaderWriter::WriteResult rr = itr->writeHeightField(HeightField,fileName,options);
            if (rr.success()) return rr;
            else results.push_back(rr);
        }
    }

    if (results.empty())
    {
        return ReaderWriter::WriteResult("Warning: Could not find plugin to write HeightField to file \""+fileName+"\".");
    }

    if (results.front().message().empty())
    {
        switch(results.front().status())
        {
            case(ReaderWriter::WriteResult::FILE_NOT_HANDLED): results.front().message() = "Warning: Write to \""+fileName+"\" not supported."; break;
            case(ReaderWriter::WriteResult::ERROR_IN_WRITING_FILE): results.front().message() = "Warning: Error in writing to \""+fileName+"\"."; break;
            default: break;
        }
    }
    
    return results.front();
}


ReaderWriter::ReadResult Registry::readNodeImplementation(const std::string& fileName,const Options* options)
{
#if 0
    osg::Timer_t startTick = osg::Timer::instance()->tick();
    ReaderWriter::ReadResult result = readImplementation(ReadNodeFunctor(fileName, options),Options::CACHE_NODES);
    osg::Timer_t endTick = osg::Timer::instance()->tick();
    OSG_NOTICE<<"time to load "<<fileName<<" "<<osg::Timer::instance()->delta_m(startTick, endTick)<<"ms"<<std::endl;
    return result;
#else

    return readImplementation(ReadNodeFunctor(fileName, options));
                              
#endif
}
ReaderWriter::ReadResult Registry::readNodeImplementation(const ID& id,const Options* options)
{
#if 0
    osg::Timer_t startTick = osg::Timer::instance()->tick();
    ReaderWriter::ReadResult result = readImplementation(ReadNodeFunctor(fileName, options),Options::CACHE_NODES);
    osg::Timer_t endTick = osg::Timer::instance()->tick();
    OSG_NOTICE<<"time to load "<<fileName<<" "<<osg::Timer::instance()->delta_m(startTick, endTick)<<"ms"<<std::endl;
    return result;
#else

    return readImplementation(ReadNodeFunctor(id, options));

#endif
}

ReaderWriter::WriteResult Registry::writeNodeImplementation(const Node& node,const std::string& fileName,const Options* options)
{
    // record the errors reported by readerwriters.
    typedef std::vector<ReaderWriter::WriteResult> Results;
    Results results;

    // first attempt to write the file from existing ReaderWriter's
    AvailableReaderWriterIterator itr(_rwList, _pluginMutex);
    for(;itr.valid();++itr)
    {
        ReaderWriter::WriteResult rr = itr->writeNode(node,fileName,options);
        if (rr.success()) return rr;
        else results.push_back(rr);
    }

    results.clear();

    // now look for a plug-in to save the file.
    std::string libraryName = createLibraryNameForFile(fileName);


    if (loadLibrary(libraryName)==LOADED)
    {
        for(;itr.valid();++itr)
        {
            ReaderWriter::WriteResult rr = itr->writeNode(node,fileName,options);
    
            if (rr.success()) return rr;
            else results.push_back(rr);
        }
    }

    if (results.empty())
    {
        return ReaderWriter::WriteResult("Warning: Could not find plugin to write nodes to file \""+fileName+"\".");
    }

    if (results.front().message().empty())
    {
        switch(results.front().status())
        {
            case(ReaderWriter::WriteResult::FILE_NOT_HANDLED): results.front().message() = "Warning: Write to \""+fileName+"\" not supported."; break;
            case(ReaderWriter::WriteResult::ERROR_IN_WRITING_FILE): results.front().message() = "Warning: Error in writing to \""+fileName+"\"."; break;
            default: break;
        }
    }

    return results.front();
}

ReaderWriter::ReadResult Registry::readShaderImplementation(const std::string& fileName,const Options* options)
{
    return readImplementation(ReadShaderFunctor(fileName, options));
}
ReaderWriter::ReadResult Registry::readShaderImplementation(const ID& id,const Options* options)
{
    return readImplementation(ReadShaderFunctor(id, options));
}

ReaderWriter::WriteResult Registry::writeShaderImplementation(const Shader& shader,const std::string& fileName,const Options* options)
{
    // record the errors reported by readerwriters.
    typedef std::vector<ReaderWriter::WriteResult> Results;
    Results results;

    // first attempt to load the file from existing ReaderWriter's
    AvailableReaderWriterIterator itr(_rwList, _pluginMutex);
    for(;itr.valid();++itr)
    {
        ReaderWriter::WriteResult rr = itr->writeShader(shader,fileName,options);
        if (rr.success()) return rr;
        else results.push_back(rr);
    }

    results.clear();

    // now look for a plug-in to save the file.
    std::string libraryName = createLibraryNameForFile(fileName);
    if (loadLibrary(libraryName)==LOADED)
    {
        for(;itr.valid();++itr)
        {
            ReaderWriter::WriteResult rr = itr->writeShader(shader,fileName,options);
            if (rr.success()) return rr;
            else results.push_back(rr);
        }
    }

    if (results.empty())
    {
        return ReaderWriter::WriteResult("Warning: Could not find plugin to write shader to file \""+fileName+"\".");
    }
    
    if (results.front().message().empty())
    {
        switch(results.front().status())
        {
            case(ReaderWriter::WriteResult::FILE_NOT_HANDLED): results.front().message() = "Warning: Write to \""+fileName+"\" not supported."; break;
            case(ReaderWriter::WriteResult::ERROR_IN_WRITING_FILE): results.front().message() = "Warning: Error in writing to \""+fileName+"\"."; break;
            default: break;
        }
    }

    return results.front();
}

void Registry::addEntryToObjectCache(const std::string& filename, osg::Object* object, double timestamp)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    _objectCache[filename]=ObjectTimeStampPair(object,timestamp);
}

osg::Object* Registry::getFromObjectCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    ObjectCache::iterator itr = _objectCache.find(fileName);
    if (itr!=_objectCache.end()) return itr->second.first.get();
    else return 0;
}

osg::ref_ptr<osg::Object> Registry::getRefFromObjectCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    ObjectCache::iterator itr = _objectCache.find(fileName);
    if (itr!=_objectCache.end()) return itr->second.first;
    else return 0;
}

void Registry::updateTimeStampOfObjectsInCacheWithExternalReferences(const osg::FrameStamp& frameStamp)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

    // look for objects with external references and update their time stamp.
    for(ObjectCache::iterator itr=_objectCache.begin();
        itr!=_objectCache.end();
        ++itr)
    {
        // if ref count is greater the 1 the object has an external reference.
        if (itr->second.first->referenceCount()>1)
        {
            // so update it time stamp.
            itr->second.second = frameStamp.getReferenceTime();
        }
    }
}

void Registry::removeExpiredObjectsInCache(const osg::FrameStamp& frameStamp)
{
    double expiryTime = frameStamp.getReferenceTime() - _expiryDelay;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

    typedef std::vector<std::string> ObjectsToRemove;
    ObjectsToRemove objectsToRemove;

    // first collect all the expired entries in the ObjectToRemove list.
    for(ObjectCache::iterator oitr=_objectCache.begin();
        oitr!=_objectCache.end();
        ++oitr)
    {
        if (oitr->second.second<=expiryTime)
        {
            // record the filename of the entry to use as key for deleting
            // afterwards/
            objectsToRemove.push_back(oitr->first);
        }
    }
    
    // remove the entries from the _objectCaache.
    for(ObjectsToRemove::iterator ritr=objectsToRemove.begin();
        ritr!=objectsToRemove.end();
        ++ritr)
    {
        // std::cout<<"Removing from Registry object cache '"<<*ritr<<"'"<<std::endl;
        _objectCache.erase(*ritr);
    }
        
}

void Registry::clearObjectCache()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    _objectCache.clear();
}

void Registry::addToArchiveCache(const std::string& fileName, osgDB::Archive* archive)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_archiveCacheMutex);
    _archiveCache[fileName] = archive;
}

/** Remove archive from cache.*/
void Registry::removeFromArchiveCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_archiveCacheMutex);
    ArchiveCache::iterator itr = _archiveCache.find(fileName);
    if (itr!=_archiveCache.end()) 
    {
        _archiveCache.erase(itr);
    }
}

osgDB::Archive* Registry::getFromArchiveCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_archiveCacheMutex);
    ArchiveCache::iterator itr = _archiveCache.find(fileName);
    if (itr!=_archiveCache.end()) return itr->second.get();
    else return 0;
}

osg::ref_ptr<osgDB::Archive> Registry::getRefFromArchiveCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_archiveCacheMutex);
    ArchiveCache::iterator itr = _archiveCache.find(fileName);
    if (itr!=_archiveCache.end()) return itr->second;
    else return 0;
}

void Registry::clearArchiveCache()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_archiveCacheMutex);
    _archiveCache.clear();
}

void Registry::releaseGLObjects(osg::State* state)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

    for(ObjectCache::iterator itr = _objectCache.begin();
        itr != _objectCache.end();
        ++itr)
    {
        osg::Object* object = itr->second.first.get();
        object->releaseGLObjects(state);
    }

    if (_sharedStateManager.valid())
    {
      _sharedStateManager->releaseGLObjects( state );
    }
}

SharedStateManager* Registry::getOrCreateSharedStateManager()
{
    if (!_sharedStateManager) _sharedStateManager = new SharedStateManager;
    
    return _sharedStateManager.get();
}
