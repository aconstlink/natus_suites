

#include <natus/application/global.h>
#include <natus/application/app.h>

#include <natus/format/global.h>
#include <natus/format/future_items.hpp>
#include <natus/format/natus/natus_module.h>

#include <natus/io/database.h>

struct sprite_sheet
{
    struct animation
    {
        struct sprite
        {
            size_t idx ;
            size_t begin ;
            size_t end ;
        };
        size_t duration ;
        natus::ntd::vector< sprite > sprites ;

    };
    natus::ntd::vector< animation > animations ;

    struct sprite
    {
        natus::math::vec4f_t rect ;
        natus::math::vec2f_t pivot ;
    };
    natus::ntd::vector< sprite > rects ;
};
natus::ntd::vector< sprite_sheet > sheets ;

int main( int argc, char ** argv )
{
    natus::io::database_res_t db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ; 

    // import natus file
    {
        natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
        auto item = mod_reg->import_from( natus::io::location_t( "sprite_sheet.natus" ), db ) ;

        natus::format::natus_item_res_t ni = item.get() ;
        if( ni.is_valid() )
        {
            natus::format::natus_document_t doc = std::move( ni->doc ) ;

            // taking all slices
            natus::graphics::image_t imgs ;

            // load images
            {
                natus::ntd::vector< natus::format::future_item_t > futures ;
                for( auto const & ss : doc.sprite_sheets )
                {
                    auto const l = natus::io::location_t::from_path( natus::io::path_t(ss.image.src) ) ;
                    futures.emplace_back( mod_reg->import_from( l, db ) ) ;
                }

                for( size_t i=0; i<doc.sprite_sheets.size(); ++i )
                {
                    natus::format::image_item_res_t ii = futures[i].get() ;
                    if( ii.is_valid() )
                    {
                        imgs.append( *ii->img ) ;
                    }

                    sprite_sheet ss ;
                    sheets.emplace_back( ss ) ;
                }
            }

            // make sprite animation infos
            {
                // as an image array is used, the max dims need to be
                // used to compute the particular rect infos
                auto dims = imgs.get_dims() ;
                    
                size_t i=0 ;
                for( auto const & ss : doc.sprite_sheets )
                {
                    auto & sheet = sheets[i++] ;

                    for( auto const & s : ss.sprites )
                    {
                        natus::math::vec4f_t const rect = 
                            (natus::math::vec4f_t( s.animation.rect ) + 
                                natus::math::vec4f_t(0.5f,0.5f, 0.5f, 0.5f))/ 
                            natus::math::vec4f_t( dims.xy(), dims.xy() )  ;

                        natus::math::vec2f_t const pivot =
                            natus::math::vec2f_t( s.animation.pivot ) / 
                            natus::math::vec2f_t( dims.xy() ) ;

                        sprite_sheet::sprite s_ ;
                        s_.rect = rect ;
                        s_.pivot = pivot ;

                        sheet.rects.emplace_back( s_ ) ;
                    }

                    for( auto const & a : ss.animations )
                    {
                        sprite_sheet::animation a_ ;

                        size_t tp = 0 ;
                        for( auto const & f : a.frames )
                        {
                            auto iter = std::find_if( ss.sprites.begin(), ss.sprites.end(), 
                                [&]( natus::format::natus_document_t::sprite_sheet_t::sprite_cref_t s )
                            {
                                return s.name == f.sprite ;
                            } ) ;

                            size_t const d = std::distance( ss.sprites.begin(), iter ) ;
                            sprite_sheet::animation::sprite s_ ;
                            s_.begin = tp ;
                            s_.end = tp + f.duration ;
                            s_.idx = d ;
                            a_.sprites.emplace_back( s_ ) ;

                            tp = s_.end ;
                        }
                        a_.duration = tp ;
                        sheet.animations.emplace_back( std::move( a_ ) ) ;
                    }
                }
            }
        }
    }

    // export 
    {
        natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;

        natus::format::natus_document doc ;
        
        for( size_t no = 0; no < 3; ++no )
        {
            natus::format::natus_document_t::sprite_sheet_t ss ;
            ss.name = "sprite_sheet_name." + std::to_string(no) ;

            {
                 natus::format::natus_document_t::sprite_sheet_t::image_t img ;
                 img.src = "images/a_img."+std::to_string(no)+".png" ;
                 ss.image = img ;
            }

            for( size_t i=0; i<10; ++i )
            {
                natus::format::natus_document_t::sprite_sheet_t::sprite_t sp ;
                sp.name = "sprite." + std::to_string(i) ;
                sp.animation.rect = natus::math::vec4ui_t() ;
                sp.animation.pivot = natus::math::vec2i_t() ;

                ss.sprites.emplace_back( sp ) ;
            }

            for( size_t i=0; i<3; ++i )
            {
                natus::format::natus_document_t::sprite_sheet_t::animation_t ani ;
                ani.name = "animation." + std::to_string(i) ;

                for( size_t j=0; j<10; ++j )
                {
                    natus::format::natus_document_t::sprite_sheet_t::animation_t::frame_t fr ;
                    fr.sprite = "sprite." +std::to_string(j) ;
                    fr.duration = 50 ;
                    ani.frames.emplace_back( std::move( fr ) ) ;
                }

                ss.animations.emplace_back( std::move( ani ) ) ;
            }

            doc.sprite_sheets.emplace_back( ss ) ;
        }
        
        auto item = mod_reg->export_to( natus::io::location_t( "sprite_sheet_export.natus" ), db, 
            natus::format::natus_item_res_t( natus::format::natus_item_t( std::move( doc ) ) ) ) ;

        natus::format::status_item_res_t r =  item.get() ;
    }

    // import export the same file
    {
        natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
        auto item = mod_reg->import_from( natus::io::location_t( "sprite_sheet.natus" ), db ) ;

        natus::format::natus_item_res_t ni = item.get() ;
        if( ni.is_valid() )
        {
            natus::format::natus_document_t doc = std::move( ni->doc ) ;
            auto exp_item = mod_reg->export_to( natus::io::location_t( "imported_one.natus" ), db, 
            natus::format::natus_item_res_t( natus::format::natus_item_t( std::move( doc ) ) ) ) ;

            natus::format::status_item_res_t r =  exp_item.get() ;
        }
    }

    return 0 ;
}
 