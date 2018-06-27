/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


// Not Apple or Windows
#if not defined(__APPLE__) && not defined(_WIN32)
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <GL/glx.h>
#endif

#ifndef _WIN32
  #include <dirent.h>
#else
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
  #include <ignition/common/win_dirent.h>
#endif
#include <ignition/common/Console.hh>
#include <ignition/common/Filesystem.hh>
#include <ignition/common/Util.hh>

#include "ignition/rendering/RenderEngineManager.hh"
#include "ignition/rendering/ogre2/Ogre2Includes.hh"
#include "ignition/rendering/ogre2/Ogre2RenderEngine.hh"
#include "ignition/rendering/ogre2/Ogre2RenderTypes.hh"
//#include "ignition/rendering/ogre2/Ogre2Scene.hh"
//#include "ignition/rendering/ogre2/OgreStorage.hh"

namespace ignition
{
  namespace rendering
  {
    class Ogre2RenderEnginePrivate
    {
#if not defined(__APPLE__) && not defined(_WIN32)
      public: XVisualInfo *dummyVisual = nullptr;
#endif
    };
  }
}

using namespace ignition;
using namespace rendering;

//////////////////////////////////////////////////
Ogre2RenderEnginePlugin::Ogre2RenderEnginePlugin()
{
}

//////////////////////////////////////////////////
std::string Ogre2RenderEnginePlugin::Name() const
{
  return Ogre2RenderEngine::Instance()->Name();
}

//////////////////////////////////////////////////
RenderEngine *Ogre2RenderEnginePlugin::Engine() const
{
  return Ogre2RenderEngine::Instance();
}

//////////////////////////////////////////////////
Ogre2RenderEngine::Ogre2RenderEngine() :
  dataPtr(new Ogre2RenderEnginePrivate)
{
#if not (__APPLE__ || _WIN32)
  this->dummyDisplay = nullptr;
  this->dummyContext = 0;
#endif

  this->dummyWindowId = 0;

  this->ogrePaths.push_back(std::string(OGRE2_RESOURCE_PATH));
}

//////////////////////////////////////////////////
Ogre2RenderEngine::~Ogre2RenderEngine()
{
}

//////////////////////////////////////////////////
bool Ogre2RenderEngine::Fini()
{
/*  if (this->scenes)
  {
    this->scenes->RemoveAll();
  }
  */

#if not (__APPLE__ || _WIN32)
  if (this->dummyDisplay)
  {
    Display *x11Display = static_cast<Display*>(this->dummyDisplay);
    GLXContext x11Context = static_cast<GLXContext>(this->dummyContext);
    glXDestroyContext(x11Display, x11Context);
    XDestroyWindow(x11Display, this->dummyWindowId);
    XCloseDisplay(x11Display);
    this->dummyDisplay = nullptr;
    XFree(this->dataPtr->dummyVisual);
    this->dataPtr->dummyVisual = nullptr;
  }
#endif

  delete this->ogreOverlaySystem;
  this->ogreOverlaySystem = nullptr;

  if (ogreRoot)
  {
    this->ogreRoot->shutdown();
    // TODO: fix segfault on delete
    // delete this->ogreRoot;
    this->ogreRoot = nullptr;
  }

  delete this->ogreLogManager;
  this->ogreLogManager = nullptr;

  this->loaded = false;
  this->initialized = false;

  return true;
}

//////////////////////////////////////////////////
bool Ogre2RenderEngine::IsEnabled() const
{
  return this->initialized;
}

//////////////////////////////////////////////////
std::string Ogre2RenderEngine::Name() const
{
  return "ogre2";
}

/*//////////////////////////////////////////////////
Ogre2RenderEngine::Ogre2RenderPathType
    Ogre2RenderEngine::RenderPathType() const
{
  return this->renderPathType;
}
*/

//////////////////////////////////////////////////
/*void Ogre2RenderEngine::AddResourcePath(const std::string &_uri)
{
  if (_uri == "__default__" || _uri.empty())
    return;

  std::string path = common::findFilePath(_uri);

  if (path.empty())
  {
    ignerr << "URI doesn't exist[" << _uri << "]\n";
    return;
  }

  this->resourcePaths.push_back(path);

  try
  {
    if (!Ogre::ResourceGroupManager::getSingleton().resourceLocationExists(
          path, "General"))
    {
      Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
          path, "FileSystem", "General", true);

      Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(
          "General");
      // Parse all material files in the path if any exist
      if (common::isDirectory(path))
      {
        std::vector<std::string> paths;

        common::DirIter endIter;
        for (common::DirIter dirIter(path); dirIter != endIter; ++dirIter)
        {
          paths.push_back(*dirIter);
        }
        std::sort(paths.begin(), paths.end());

        // Iterate over all the models in the current ign-rendering path
        for (auto dIter = paths.begin(); dIter != paths.end(); ++dIter)
        {
          std::string fullPath = *dIter;
          std::string matExtension = fullPath.substr(fullPath.size()-9);
          if (matExtension == ".material")
          {
            Ogre::DataStreamPtr stream =
              Ogre::ResourceGroupManager::getSingleton().openResource(
                  fullPath, "General");

            // There is a material file under there somewhere, read the thing in
            try
            {
              Ogre::MaterialManager::getSingleton().parseScript(
                  stream, "General");
              Ogre::MaterialPtr matPtr =
                Ogre::MaterialManager::getSingleton().getByName(
                    fullPath);

              if (!matPtr.isNull())
              {
                // is this necessary to do here? Someday try it without
                matPtr->compile();
                matPtr->load();
              }
            }
            catch(Ogre::Exception& e)
            {
              ignerr << "Unable to parse material file[" << fullPath << "]\n";
            }
            stream->close();
          }
        }
      }
    }
  }
  catch(Ogre::Exception &_e)
  {
    ignerr << "Unable to load Ogre Resources.\nMake sure the"
        "resources path in the world file is set correctly." << std::endl;
  }
}
  */

//////////////////////////////////////////////////
Ogre::Root *Ogre2RenderEngine::OgreRoot() const
{
  return this->ogreRoot;
}

//////////////////////////////////////////////////
ScenePtr Ogre2RenderEngine::CreateSceneImpl(unsigned int _id,
    const std::string &_name)
{
//  auto scene = OgreScenePtr(new OgreScene(_id, _name));
//  this->scenes->Add(scene);
//  return scene;
  return ScenePtr();
}

//////////////////////////////////////////////////
SceneStorePtr Ogre2RenderEngine::Scenes() const
{
//  return this->scenes;
  return SceneStorePtr();
}

//////////////////////////////////////////////////
bool Ogre2RenderEngine::LoadImpl()
{
  try
  {
    this->LoadAttempt();
    this->loaded = true;
    return true;
  }
  catch (Ogre::Exception &ex)
  {
    ignerr << ex.what() << std::endl;
    return false;
  }
  catch (...)
  {
    ignerr << "Failed to load render-engine" << std::endl;
    return false;
  }
}

//////////////////////////////////////////////////
bool Ogre2RenderEngine::InitImpl()
{
  try
  {
    this->InitAttempt();
    return true;
  }
  catch (...)
  {
    ignerr << "Failed to initialize render-engine" << std::endl;
    return false;
  }
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::LoadAttempt()
{
  this->CreateLogger();
  this->CreateContext();
  this->CreateRoot();
  this->CreateOverlay();
  this->LoadPlugins();
  this->CreateRenderSystem();
  this->ogreRoot->initialise(false);
  this->CreateWindow();
  this->CreateResources();
  //this->CheckCapabilities();
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateLogger()
{
  // create log file path
  std::string logPath;
  ignition::common::env(IGN_HOMEDIR, logPath);
  logPath = common::joinPaths(logPath, ".ignition", "rendering");
  common::createDirectories(logPath);
  logPath = common::joinPaths(logPath, "ogre2.log");

  // create actual log
  this->ogreLogManager = new Ogre::LogManager();
  this->ogreLogManager->createLog(logPath, true, false, false);
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateContext()
{
#if not (__APPLE__ || _WIN32)
  // create X11 display
  this->dummyDisplay = XOpenDisplay(0);
  Display *x11Display = static_cast<Display*>(this->dummyDisplay);

  if (!this->dummyDisplay)
  {
    ignerr << "Unable to open display: " << XDisplayName(0) << std::endl;
    return;
  }

  // create X11 visual
  int screenId = DefaultScreen(x11Display);

  int attributeList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16,
      GLX_STENCIL_SIZE, 8, None };

  this->dataPtr->dummyVisual =
      glXChooseVisual(x11Display, screenId, attributeList);

  if (!this->dataPtr->dummyVisual)
  {
    ignerr << "Unable to create glx visual" << std::endl;
    return;
  }

  // create X11 context
  this->dummyWindowId = XCreateSimpleWindow(x11Display,
      RootWindow(this->dummyDisplay, screenId), 0, 0, 1, 1, 0, 0, 0);

  this->dummyContext = glXCreateContext(x11Display, this->dataPtr->dummyVisual,
                                        nullptr, 1);

  GLXContext x11Context = static_cast<GLXContext>(this->dummyContext);

  if (!this->dummyContext)
  {
    ignerr << "Unable to create glx context" << std::endl;
    return;
  }

  // select X11 context
  glXMakeCurrent(x11Display, this->dummyWindowId, x11Context);
#endif
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateRoot()
{
  try
  {
    this->ogreRoot = new Ogre::Root();
  }
  catch (Ogre::Exception &ex)
  {
    ignerr << "Unable to create Ogre root" << std::endl;
  }
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateOverlay()
{
  this->ogreOverlaySystem = new Ogre::v1::OverlaySystem();
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::LoadPlugins()
{
  for (auto iter = this->ogrePaths.begin();
       iter != this->ogrePaths.end(); ++iter)
  {
    std::string path(*iter);
    if (!common::isDirectory(path))
      continue;

    std::vector<std::string> plugins;
    std::vector<std::string>::iterator piter;

#ifdef __APPLE__
    std::string prefix = "lib";
    std::string extension = ".dylib";
#else
    std::string prefix = "";
    std::string extension = ".so";
#endif

    plugins.push_back(path+"/"+prefix+"RenderSystem_GL3Plus");
    plugins.push_back(path+"/"+prefix+"Plugin_ParticleFX");

    for (piter = plugins.begin(); piter != plugins.end(); ++piter)
    {
      try
      {
        // Load the plugin into OGRE
        this->ogreRoot->loadPlugin(*piter+extension);
      }
      catch(Ogre::Exception &e)
      {
        try
        {
          // Load the debug plugin into OGRE
          this->ogreRoot->loadPlugin(*piter+"_d"+extension);
        }
        catch(Ogre::Exception &ed)
        {
          if ((*piter).find("RenderSystem") != std::string::npos)
          {
            ignerr << "Unable to load Ogre Plugin[" << *piter
                  << "]. Rendering will not be possible."
                  << "Make sure you have installed OGRE and Gazebo properly.\n";
          }
        }
      }
    }
  }
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateRenderSystem()
{
  Ogre::RenderSystem *renderSys;
  const Ogre::RenderSystemList *rsList;

  // Set parameters of render system (window size, etc.)
#if  OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
  rsList = this->ogreRoot->getAvailableRenderers();
#else
  rsList = &(this->ogreRoot->getAvailableRenderers());
#endif

  int c = 0;

  renderSys = nullptr;

  do
  {
    if (c == static_cast<int>(rsList->size()))
      break;

    renderSys = rsList->at(c);
    c++;
  }
  while (renderSys &&
         renderSys->getName().compare("OpenGL 3+ Rendering Subsystem") != 0);

  if (renderSys == nullptr)
  {
    ignerr << "unable to find OpenGL rendering system. OGRE is probably "
            "installed incorrectly. Double check the OGRE cmake output, "
            "and make sure OpenGL is enabled." << std::endl;
  }

  // We operate in windowed mode
  renderSys->setConfigOption("Full Screen", "No");

  /// We used to allow the user to set the RTT mode to PBuffer, FBO, or Copy.
  ///   Copy is slow, and there doesn't seem to be a good reason to use it
  ///   PBuffer limits the size of the renderable area of the RTT to the
  ///           size of the first window created.
  ///   FBO seem to be the only good option
  renderSys->setConfigOption("RTT Preferred Mode", "FBO");

//  renderSys->setConfigOption("FSAA", "4");

  this->ogreRoot->setRenderSystem(renderSys);
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateResources()
{

  // TODO support loading resources from user specified paths
  std::list<std::string> paths;
  const char *env = std::getenv("IGN_RENDERING_RESOURCE_PATH");
  std::string resourcePath = (env) ? std::string(env) :
      IGN_RENDERING_RESOURCE_PATH;
  // install path
  std::string mediaPath = common::joinPaths(resourcePath, "ogre2", "media");
  if (!common::exists(mediaPath))
  {
    // src path
    mediaPath = common::joinPaths(resourcePath, "ogre2", "src", "media");
  }

  Ogre::String rootHlmsFolder = mediaPath;

  if( rootHlmsFolder.empty() )
      rootHlmsFolder = "./";
  else if( *(rootHlmsFolder.end() - 1) != '/' )
      rootHlmsFolder += "/";

  //At this point rootHlmsFolder should be a valid path to the Hlms data folder

  Ogre::HlmsUnlit *hlmsUnlit = 0;
  Ogre::HlmsPbs *hlmsPbs = 0;

  //For retrieval of the paths to the different folders needed
  Ogre::String mainFolderPath;
  Ogre::StringVector libraryFoldersPaths;
  Ogre::StringVector::const_iterator libraryFolderPathIt;
  Ogre::StringVector::const_iterator libraryFolderPathEn;

  Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
  
  {
    //Create & Register HlmsUnlit
    //Get the path to all the subdirectories used by HlmsUnlit
    Ogre::HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
    Ogre::Archive *archiveUnlit = archiveManager.load( rootHlmsFolder + mainFolderPath,
                                                       "FileSystem", true );
    Ogre::ArchiveVec archiveUnlitLibraryFolders;
    libraryFolderPathIt = libraryFoldersPaths.begin();
    libraryFolderPathEn = libraryFoldersPaths.end();
    while( libraryFolderPathIt != libraryFolderPathEn )
    {
        Ogre::Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
        archiveUnlitLibraryFolders.push_back( archiveLibrary );
        ++libraryFolderPathIt;
    }

    //Create and register the unlit Hlms
    hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
    Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
  }

  {
    //Create & Register HlmsPbs
    //Do the same for HlmsPbs:
    Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
    Ogre::Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath,
                                                     "FileSystem", true );

    //Get the library archive(s)
    Ogre::ArchiveVec archivePbsLibraryFolders;
    libraryFolderPathIt = libraryFoldersPaths.begin();
    libraryFolderPathEn = libraryFoldersPaths.end();
    while( libraryFolderPathIt != libraryFolderPathEn )
    {
        Ogre::Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
        archivePbsLibraryFolders.push_back( archiveLibrary );
        ++libraryFolderPathIt;
    }

    //Create and register
    hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
    Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
  }
}

//////////////////////////////////////////////////
void Ogre2RenderEngine::CreateWindow()
{
  // create dummy window
  this->CreateWindow(std::to_string(this->dummyWindowId), 1, 1, 1, 0);
}

//////////////////////////////////////////////////
std::string Ogre2RenderEngine::CreateWindow(const std::string &_handle,
    const unsigned int _width, const unsigned int _height,
    const double _ratio, const unsigned int _antiAliasing)
{
  Ogre::StringVector paramsVector;
  Ogre::NameValuePairList params;
  Ogre::RenderWindow *window = nullptr;

  // Mac and Windows *must* use externalWindow handle.
#if defined(__APPLE__) || defined(_MSC_VER)
  params["externalWindowHandle"] = _handle;
#else
  params["parentWindowHandle"] = _handle;
#endif
  params["FSAA"] = std::to_string(_antiAliasing);
  params["stereoMode"] = "Frame Sequential";

  // TODO: determine api without qt

  // Set the macAPI for Ogre based on the Qt implementation
  params["macAPI"] = "cocoa";
  params["macAPICocoaUseNSView"] = "true";

  // Hide window if dimensions are less than or equal to one.
  params["border"] = "none";

  std::ostringstream stream;
  stream << "OgreWindow(0)" << "_" << _handle;

  // Needed for retina displays
  params["contentScalingFactor"] = std::to_string(_ratio);

  int attempts = 0;
  while (window == nullptr && (attempts++) < 10)
  {
    try
    {
      window = this->ogreRoot->createRenderWindow(
          stream.str(), _width, _height, false, &params);
    }
    catch(...)
    {
      ignerr << " Unable to create the rendering window\n";
      window = nullptr;
    }
  }

  if (attempts >= 10)
  {
    ignerr << "Unable to create the rendering window\n" << std::endl;
    return std::string();
  }

  if (window)
  {
    window->setActive(true);
    window->setVisible(true);
//    window->setAutoUpdated(false);

    // Windows needs to reposition the render window to 0,0.
    window->reposition(0, 0);
  }
  return stream.str();
}

//////////////////////////////////////////////////
/*void Ogre2RenderEngine::CheckCapabilities()
{
  const Ogre::RenderSystemCapabilities *capabilities;
  Ogre::RenderSystemCapabilities::ShaderProfiles profiles;
  Ogre::RenderSystemCapabilities::ShaderProfiles::const_iterator iter;

  capabilities = this->ogreRoot->getRenderSystem()->getCapabilities();
  profiles = capabilities->getSupportedShaderProfiles();

  bool hasFragmentPrograms =
    capabilities->hasCapability(Ogre::RSC_FRAGMENT_PROGRAM);

  bool hasVertexPrograms =
    capabilities->hasCapability(Ogre::RSC_VERTEX_PROGRAM);

  bool hasFBO =
    capabilities->hasCapability(Ogre::RSC_FBO);

  bool hasGLSL =
    std::find(profiles.begin(), profiles.end(), "glsl") != profiles.end();

  if (!hasFragmentPrograms || !hasVertexPrograms)
    ignwarn << "Vertex and fragment shaders are missing. "
           << "Fixed function rendering will be used.\n";

  if (!hasGLSL)
    ignwarn << "GLSL is missing."
           << "Fixed function rendering will be used.\n";

  if (!hasFBO)
    ignwarn << "Frame Buffer Objects (FBO) is missing. "
           << "Rendering will be disabled.\n";

  this->renderPathType = Ogre2RenderEngine::NONE;

  if (hasFBO && hasGLSL && hasVertexPrograms && hasFragmentPrograms)
  {
    this->renderPathType = Ogre2RenderEngine::FORWARD;
  }
  else if (hasFBO)
  {
    this->renderPathType = Ogre2RenderEngine::VERTEX;
  }
}
*/

//////////////////////////////////////////////////
void Ogre2RenderEngine::InitAttempt()
{
/*  if (this->renderPathType == NONE)
  {
    ignwarn << "Cannot initialize render engine since "
           << "render path type is NONE. Ignore this warning if"
           << "rendering has been turned off on purpose.\n";
    return;
  }
  */

  this->initialized = false;

  Ogre::ColourValue ambient;

  /// Create a dummy rendering context.
  /// This will allow ign-rendering to run headless. And it also allows OGRE to
  /// initialize properly

  // Set default mipmap level (NB some APIs ignore this)
/*  Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

  // init the resources
  Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

  Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(
      Ogre::TFO_ANISOTROPIC);

  OgreRTShaderSystem::Instance()->Init();

  this->scenes = OgreSceneStorePtr(new OgreSceneStore);
  */
}

/////////////////////////////////////////////////
/*Ogre::OverlaySystem *Ogre2RenderEngine::OverlaySystem() const
{
  return this->ogreOverlaySystem;
}
*/

// Register this plugin
IGN_COMMON_REGISTER_SINGLE_PLUGIN(ignition::rendering::Ogre2RenderEnginePlugin,
                                  ignition::rendering::RenderEnginePlugin)
