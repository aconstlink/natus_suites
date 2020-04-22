
#include "main.h"

#include <natus/application/global.h>

#include <thread>

#if defined( NATUS_GRAPHICS_WGL )

#include <natus/application/platform/wgl/wgl_window.h>
#include <natus/application/platform/win32/win32_application.h>

#elif defined( NATUS_GRAPHICS_GLX )

#endif

int main( int argc, char ** argv )
{
    natus::application::gl_info_t gli ;
    {
        gli.vsync_enabled = true ;
    }

    natus::application::window_info_t wi ;
    {
        wi.w = 800 ;
        wi.h = 800 ;
        wi.window_name = "Render Window Test" ;
    }

#if defined( NATUS_GRAPHICS_WGL )

    natus::application::wgl::window_res_t wglw0 =
        natus::application::wgl::window_t( gli, wi ) ;
    
    //natus::application::wgl::window_res_t wglw1 =
      //  natus::application::wgl::window_t( gli, wi ) ;

    natus::application::win32::win32_application_t app ;
    app.exec() ;

#elif defined( NATUS_GRAPHICS_GLX )
#endif

    return 0 ;
}
