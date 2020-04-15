
#include "main.h"

#include <natus/application/global.h>

#include <thread>

#if defined( NATUS_GRAPHICS_WGL )

#include <natus/application/platform/win32/win32_application.h>

#elif defined( NATUS_GRAPHICS_GLX )

#endif

int main( int argc, char ** argv )
{

#if defined( NATUS_GRAPHICS )

    natus::application::wgl::window_res_t wglw = 
        natus::application::wgl::window_t( gli, wi ) ;

    natus::appli

#elif defined( NATUS_GRAPHICS_GLX )
#endif

    std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) ) ;
    return 0 ;
}
