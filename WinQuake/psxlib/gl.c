#include "gl.h"

void glClear( GLbitfield mask )
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

void glBegin(void)
{

}

void glEnd(void)
{

}

void glVertex3f( GLfloat x, GLfloat y, GLfloat z )
{

}

void glTexCoord2f( GLfloat s, GLfloat t )
{

}

void glVertex2f( GLfloat x, GLfloat y )
{

}

void glVertex3fv( const GLfloat *v )
{

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

void glPushMatrix( void )
{

}

void glPopMatrix( void )
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
