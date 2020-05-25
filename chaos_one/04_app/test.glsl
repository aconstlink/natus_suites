#version 330 core
                        in vec3 in_pos ;
                        uniform mat4 u_proj ;
                        uniform mat4 u_view ;

                        //out vertex_data
                        //{
                        //    vec2 tx ;
                        //} vso ;

                        void main()
                        {
                            gl_Position = u_proj * u_view * vec4( in_pos, 1.0 ) ;
                        }