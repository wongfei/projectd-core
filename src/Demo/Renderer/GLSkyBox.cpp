#include "Renderer/GLSkyBox.h"

namespace D {

void GLSkyBox::load(float dim, const char* path, const char* name, const char* ext)
{
	vec3f pos(0, 0, 0);
	vec3f size(dim, dim, dim);
	pos -= size * 0.5f;

	char fullpath[1024];
	for (int i = 0; i < 6; ++i)
	{
		snprintf(fullpath, 255, "%s/%s%d.%s", path, name, i, ext);
		tex[i].load(fullpath);
	}

	GLCompileScoped compile(list);

	glBindTexture(GL_TEXTURE_2D, tex[0].id); // front
	glBegin(GL_QUADS);
		glTexCoord2f(1, 1); glVertex3f(pos.x,          pos.y,          pos.z + size.z);
		glTexCoord2f(1, 0); glVertex3f(pos.x,          pos.y + size.y, pos.z + size.z);
		glTexCoord2f(0, 0); glVertex3f(pos.x + size.x, pos.y + size.y, pos.z + size.z);
		glTexCoord2f(0, 1); glVertex3f(pos.x + size.x, pos.y,          pos.z + size.z);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex[1].id); // back
	glBegin(GL_QUADS);
		glTexCoord2f(1, 1); glVertex3f(pos.x + size.x, pos.y,          pos.z);
		glTexCoord2f(1, 0); glVertex3f(pos.x + size.x, pos.y + size.y, pos.z);
		glTexCoord2f(0, 0); glVertex3f(pos.x,          pos.y + size.y, pos.z);
		glTexCoord2f(0, 1); glVertex3f(pos.x,          pos.y,          pos.z);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex[2].id); // left
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(pos.x + size.x, pos.y,          pos.z);
		glTexCoord2f(1, 1); glVertex3f(pos.x + size.x, pos.y,          pos.z + size.z);
		glTexCoord2f(1, 0); glVertex3f(pos.x + size.x, pos.y + size.y, pos.z + size.z);
		glTexCoord2f(0, 0); glVertex3f(pos.x + size.x, pos.y + size.y, pos.z);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex[3].id); // right
	glBegin(GL_QUADS);
		glTexCoord2f(1, 0); glVertex3f(pos.x, pos.y + size.y, pos.z);
		glTexCoord2f(0, 0); glVertex3f(pos.x, pos.y + size.y, pos.z + size.z);
		glTexCoord2f(0, 1); glVertex3f(pos.x, pos.y,          pos.z + size.z);
		glTexCoord2f(1, 1); glVertex3f(pos.x, pos.y,          pos.z);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex[4].id); // top
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(pos.x + size.x, pos.y + size.y, pos.z);
		glTexCoord2f(0, 1); glVertex3f(pos.x + size.x, pos.y + size.y, pos.z + size.z);
		glTexCoord2f(1, 1); glVertex3f(pos.x,          pos.y + size.y, pos.z + size.z);
		glTexCoord2f(1, 0); glVertex3f(pos.x,          pos.y + size.y, pos.z);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex[5].id); // bottom
	glBegin(GL_QUADS);
		glTexCoord2f(1, 1); glVertex3f(pos.x,          pos.y, pos.z);
		glTexCoord2f(1, 0); glVertex3f(pos.x,          pos.y, pos.z + size.z);
		glTexCoord2f(0, 0); glVertex3f(pos.x + size.x, pos.y, pos.z + size.z);
		glTexCoord2f(0, 1); glVertex3f(pos.x + size.x, pos.y, pos.z);
	glEnd();
}

void GLSkyBox::draw()
{
	glEnable(GL_TEXTURE_2D);
	glColor3ub(255, 255, 255);
		
	list.draw();

	glDisable(GL_TEXTURE_2D);
}

}
