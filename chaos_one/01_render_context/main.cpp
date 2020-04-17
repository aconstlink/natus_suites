
#include "main.h"

#include <natus/application/global.h>

#include <thread>

#if defined( NATUS_GRAPHICS_WGL )

#include <natus/application/platform/wgl/wgl_window.h>
#include <natus/application/platform/wgl/wgl_context.h>

#elif defined( NATUS_GRAPHICS_GLX )

#include <natus/application/platform/glx/glx_window.h>
#include <natus/application/platform/glx/glx_context.h>

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


    natus::application::wgl::window_res_t wglw = 
        natus::application::wgl::window_t( gli, wi ) ;

#elif defined( NATUS_GRAPHICS_GLX )

    natus::application::glx::window_res_t glxw = 
        natus::application::glx::window_t( gli, wi ) ;
#endif

    std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) ) ;
    return 0 ;
}


