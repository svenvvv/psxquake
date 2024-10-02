#include "gl.h"

#include <stdio.h>
#include <psxgte.h>
#include <inline_c.h>

void glClear(GLbitfield mask)
{

}

void glDepthMask( GLboolean flag )
{

}

void glDepthFunc( GLenum func )
{

}

void glDepthRange( GLclampd near_val, GLclampd far_val )
{
}

void glEnable( GLenum cap )
{

}

void glDisable( GLenum cap )
{

}

void glBegin(GLenum mode)
{
    // printf("glBegin\n");
}

void glEnd(void)
{
    // printf("glEnd\n");
}

void glVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
    // static int reg = 0;
    // SVECTOR vec = { x, y, z };
    //
    // switch (reg) {
    //     case 0:
    //         gte_ldv0(vec);
    //         break;
    //     case 1:
    //         gte_ldv1(vec);
    //         break;
    //     case 2:
    //         gte_ldv2(vec);
    //         break;
    // }
    //
    // reg += 1;
    // if (reg == 3) {
    //     /* Rotation, Translation and Perspective Triple */
    //     gte_rtpt();
    //
    //     /* Compute normal clip for backface culling */
    //     gte_nclip();
    //
    //     /* Get result*/
    //     gte_stopz( &p );
    //
    //     reg = 0;
    // }
}

void glTexCoord2f( GLfloat s, GLfloat t )
{

}

void glVertex2f( GLfloat x, GLfloat y )
{
    // printf("glVertex2f\n");
}

void glVertex3fv( const GLfloat *v )
{
    // printf("glVertex3fv\n");
}

void glColor3f( GLfloat red, GLfloat green, GLfloat blue )
{

}

void glColor3ubv( const GLubyte *v )
{

}

void glColor4fv( const GLfloat *v )
{
}

void glTexEnvf( GLenum target, GLenum pname, GLfloat param )
{
}

void glTexParameterf( GLenum target, GLenum pname, GLfloat param )
{

}

void glShadeModel( GLenum mode )
{
}

void glOrtho( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val )
{

}

void glFrustum( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val )
{
}

void glLoadMatrixf( const GLfloat *m )
{

}

void glLoadIdentity( void )
{
}

void glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{

}

void glBlendFunc( GLenum sfactor, GLenum dfactor )
{

}

void glLoadIdentity( void );



void glTexImage2D(GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels )
{

}

void glTexSubImage2D( GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels )
{

}

void glReadPixels( GLint x, GLint y,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type,
                                    GLvoid *pixels )
{

}

void glDrawBuffer( GLenum mode )
{

}

void glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{

}

void glMatrixMode( GLenum mode )
{

}

void glBindTexture( GLenum target, GLuint texture )
{

}

void glFinish( void )
{

}

void glCullFace( GLenum mode )
{

}

void glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{

}

void glScalef( GLfloat x, GLfloat y, GLfloat z )
{
}

void glGetFloatv( GLenum pname, GLfloat *params )
{
}

void glViewport( GLint x, GLint y,
                                    GLsizei width, GLsizei height )
{
}

void glReadBuffer( GLenum mode )
{
}

void glHint( GLenum target, GLenum mode )
{
}

double asin(double x)
{
    return 0;
}
