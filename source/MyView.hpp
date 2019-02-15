#pragma once

#include <sponza/sponza_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>
#include <unordered_map>

#include <vector>
#include <memory>

class MyView : public tygra::WindowViewDelegate
{
public:
    
    MyView();
    
    ~MyView();
    
    void setScene(const sponza::Context * scene);

private:

    void windowViewWillStart(tygra::Window * window) override;
    
    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;
    
    void windowViewRender(tygra::Window * window) override;

private:

    const sponza::Context * scene_;

	// Me from here down
	GLuint shader_program_{ 0 };

	const static GLuint kNullId = 0;

	// TODO: define values for your Vertex attributes
	int kVertexPosition = 0;
	int kVertexNormal = 1;
	int kVertexUV = 3;

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	// TODO: create a mesh structure to hold VBO ids etc.
	struct Mesh
	{
		int mesh_id{ 0 };
		// VertexBufferObject for the vertex positions
		GLuint vertex_vbo{ 0 };

		// VertexBufferObject for the elements (indices)
		GLuint element_vbo{ 0 };

		// VertexArrayObject for the shape's vertex array settings
		GLuint vao{ 0 };

		// Needed for when we draw using the vertex arrays
		int element_count{ 0 };
	};

	enum TextureIndexes {
		kDiffuseTexture = 0,
		kSpecularTexture = 1
	};

	void buildMesh(Mesh & mesh, int meshID, std::vector<Vertex> vertices, std::vector<unsigned int> elements);
	void createTexture(const std::string & path, GLuint & texID);

	// TODO: create a container of these mesh e.g.
	std::vector<Mesh> m_meshVector;
	std::unordered_map<std::string, GLuint> m_textures;
};
