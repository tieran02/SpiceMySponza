#include "MyView.hpp"
#include <sponza/sponza.hpp>
#include <tygra/FileHelper.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
//#include <cassert>

MyView::MyView()
{
}

MyView::~MyView() {
}

void MyView::setScene(const sponza::Context * scene)
{
    scene_ = scene;
}

void MyView::windowViewWillStart(tygra::Window * window)
{
    assert(scene_ != nullptr);

	GLint compile_status = GL_FALSE;

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	std::string vertex_shader_string
		= tygra::createStringFromFile("resource:///sponza_vs.glsl");
	const char * vertex_shader_code = vertex_shader_string.c_str();
	glShaderSource(vertex_shader, 1,
		(const GLchar **)&vertex_shader_code, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE)
	{
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetShaderInfoLog(vertex_shader, string_length, NULL, log);
		std::cerr << log << std::endl;
	}

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	std::string fragment_shader_string
		= tygra::createStringFromFile("resource:///sponza_fs.glsl");
	const char * fragment_shader_code = fragment_shader_string.c_str();
	glShaderSource(fragment_shader, 1,
		(const GLchar **)&fragment_shader_code, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE)
	{
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetShaderInfoLog(fragment_shader, string_length, NULL, log);
		std::cerr << log << std::endl;
	}

	// Create shader program & shader in variables
	shader_program_ = glCreateProgram();
	glAttachShader(shader_program_, vertex_shader);

	// TODO: glBindAttribLocation for all shader streamed IN variables e.g.
	glBindAttribLocation(shader_program_, kVertexPosition, "vertex_position");
	glBindAttribLocation(shader_program_, kVertexNormal, "vertex_normal");
	glBindAttribLocation(shader_program_, kVertexUV, "vertex_uv");

	glDeleteShader(vertex_shader);
	glAttachShader(shader_program_, fragment_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(shader_program_);

	GLint link_status = GL_FALSE;
	glGetProgramiv(shader_program_, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE)
	{
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetProgramInfoLog(shader_program_, string_length, NULL, log);
		std::cerr << log << std::endl;
	}

	/*
		The framework provides a builder class that allows access to all the mesh data	
	*/

	sponza::GeometryBuilder builder;
	const auto& source_meshes = builder.getAllMeshes();

	// We can loop through each mesh in the scene
	for each (const sponza::Mesh& source in source_meshes)
	{
		// Each mesh has an id that you will need to remember for later use
		// obained by calling source.getId()

		// To access the actual mesh raw data we can get the array e.g.
		const auto& positions = source.getPositionArray();
		const auto& normals = source.getNormalArray();
		const auto& elements = source.getElementArray();
		const auto& uvs = source.getTextureCoordinateArray();
		const auto& tangents = source.getTangentArray();

		std::vector<Vertex> vertices(positions.size());
		//create a vertex of each position to interleave the VBOs
		//This way we can reduce the VBOs required per mesh to only one
		for (size_t i = 0; i < positions.size(); i++)
		{
			vertices[i].position = glm::vec3(positions[i].x, positions[i].y, positions[i].z);
			vertices[i].normal = glm::vec3(normals[i].x, normals[i].y, normals[i].z);
			vertices[i].texCoord = glm::vec2(uvs[i].x, uvs[i].y);
		}

		Mesh mesh;
		//Build the mesh passing the elements and vertices
		buildMesh(mesh, source.getId(), vertices, elements);
		m_meshVector.push_back(mesh);
	}

	//create textures
	auto& materials = scene_->getAllMaterials();
	for(auto& mat : materials)
	{
		//Diffuse texture
		auto& diffusePath = mat.getDiffuseTexture();
		if (!diffusePath.empty() && m_textures.find(diffusePath) == m_textures.end())
		{
			GLuint texID = 0;
			createTexture("resource:///" + diffusePath, texID);
			m_textures[diffusePath] = texID;
		}

		//Specular Texture
		auto& specularPath = mat.getSpecularTexture();
		if (!specularPath.empty() && m_textures.find(specularPath) == m_textures.end())
		{
			GLuint texID = 0;
			createTexture("resource:///" + specularPath, texID);
			m_textures[specularPath] = texID;
		}
	}
}

void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
    glViewport(0, 0, width, height);
}

void MyView::windowViewDidStop(tygra::Window * window)
{
}

void MyView::windowViewRender(tygra::Window * window)
{
	assert(scene_ != nullptr);

	// Configure pipeline settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear buffers from previous frame
	glClearColor(0.f, 0.f, 0.25f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader_program_);
	 
	// Compute viewport
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];

	// Note: the code above is supplied for you and already works

	//Compute projection matrix
	const auto& camera = scene_->getCamera();
	glm::mat4 projection_xform = glm::perspective(glm::radians(camera.getVerticalFieldOfViewInDegrees()), aspect_ratio, camera.getNearPlaneDistance(), camera.getFarPlaneDistance());

	//Compute view matrix
	const auto& camera_pos = (const glm::vec3&)scene_->getCamera().getPosition();

	// Compute camera view matrix and combine with projection matrix
	glm::vec3 lookAtPos = (camera_pos + glm::vec3(camera.getDirection().x, camera.getDirection().y, camera.getDirection().z) * 5.0f);
	glm::mat4 view_xform = glm::lookAt(camera_pos, lookAtPos, glm::vec3(0,1,0));

	glm::mat4 view_projection = projection_xform * view_xform;

	//Sent matrices to the GPU via a uniform.
	auto ViewLocation = glGetUniformLocation(shader_program_, "view_xform");
	glUniformMatrix4fv(ViewLocation, 1, GL_FALSE, glm::value_ptr(view_xform));

	auto ProjectionLocation = glGetUniformLocation(shader_program_, "projection_xform");
	glUniformMatrix4fv(ProjectionLocation, 1, GL_FALSE, glm::value_ptr(projection_xform));

	// Get light data from scene and then plug the values into the shader
	auto lights = scene_->getAllLights();
	auto LightLocation = glGetUniformLocation(shader_program_, "Lights");
	for (size_t i = 0; i < lights.size(); i++)
	{
		//position
		std::string name = "Lights[" + std::to_string(i) + "].position";
		GLuint positionLocation = glGetUniformLocation(shader_program_, name.c_str());
		glUniform3f(positionLocation, lights[i].getPosition().x, lights[i].getPosition().y, lights[i].getPosition().z);

		//intensity
		name = "Lights[" + std::to_string(i) + "].intensity";
		GLuint intensityLocation = glGetUniformLocation(shader_program_, name.c_str());
		glUniform3f(intensityLocation, lights[i].getIntensity().x, lights[i].getIntensity().y, lights[i].getIntensity().z);

		//range
		name = "Lights[" + std::to_string(i) + "].range";
		GLuint rangeLocation = glGetUniformLocation(shader_program_, name.c_str());
		glUniform1f(rangeLocation, lights[i].getRange());
	}

	//Spot Light positioned in the center of the scene which points down and rotaes back and forth
	//position
	std::string name = "Lights[22].position";
	GLuint positionLocation = glGetUniformLocation(shader_program_, name.c_str());
	glUniform3f(positionLocation, 0, 150, -5);
	//intensity
	name = "Lights[22].intensity";
	GLuint intensityLocation = glGetUniformLocation(shader_program_, name.c_str());
	glUniform3f(intensityLocation, .6, 0.3, 0.3);
	//cone angle
	name = "Lights[22].range";
	GLuint rangeLocation = glGetUniformLocation(shader_program_, name.c_str());
	glUniform1f(rangeLocation, 25);

	//direction of the spot light
	name = "Lights[22].direction";
	GLuint directionLocation = glGetUniformLocation(shader_program_, name.c_str());
	float rotation = sin(scene_->getTimeInSeconds()) * 45;
	glUniform3f(directionLocation, rotation, -90, 0);


	//Directional Light - A small directional light with low intensity
	//position
	name = "Lights[23].position";
	positionLocation = glGetUniformLocation(shader_program_, name.c_str());
	glUniform3f(positionLocation, 0, 150, -5);
	//intensity
	name = "Lights[23].intensity";
	intensityLocation = glGetUniformLocation(shader_program_, name.c_str());
	glUniform3f(intensityLocation, 0.1, 0.15, 0.2);
	//direction
	name = "Lights[23].direction";
	directionLocation = glGetUniformLocation(shader_program_, name.c_str());
	glUniform3f(directionLocation, 0, -10, 75);

	//set ambient Intensity
	GLuint ambientIntensityLocation = glGetUniformLocation(shader_program_, "ambientIntensityColour");
	auto ambientIntensity = scene_->getAmbientLightIntensity();
	glUniform3f(ambientIntensityLocation, ambientIntensity.x, ambientIntensity.y, ambientIntensity.z);

	//set cameraPos in shader
	GLuint cameraPosLocation = glGetUniformLocation(shader_program_, "cameraPos");
	glUniform3f(cameraPosLocation, camera_pos.x, camera_pos.y, camera_pos.z);

	// Loop through your mesh container e.g.
	for (const auto& mesh : m_meshVector)
	{
		// Each mesh can be repeated in the scene so we need to ask the scene for all instances of the mesh
		// and render each instance with its own model matrix
		// To get the instances we need to use the meshId we stored earlier e.g.
		const auto& instances = scene_->getInstancesByMeshId(mesh.mesh_id);
		// loop through all instances
		for each (auto& instance in instances)
		{
			//get the transform matrix
			glm::mat4x3 transformMatrix = (glm::mat4x3&)scene_->getInstanceById(instance).getTransformationMatrix();
			//sent to shader via unifrom
			auto ProjectionViewModellocation = glGetUniformLocation(shader_program_, "projection_view_model_xform");
			glm::mat4 modelViewProjection = view_projection * (glm::mat4)transformMatrix;
			glUniformMatrix4fv(ProjectionViewModellocation, 1, GL_FALSE, glm::value_ptr(modelViewProjection));

			auto ModelLocation = glGetUniformLocation(shader_program_, "model_xform");
			glUniformMatrix4fv(ModelLocation, 1, GL_FALSE, glm::value_ptr((glm::mat4)transformMatrix));



			// Materials
			// Get material for this instance
			const auto& material_id = scene_->getInstanceById(instance).getMaterialId();
			const auto& material = scene_->getMaterialById(material_id);

			GLuint matAmbientLocation = glGetUniformLocation(shader_program_, "mat.ambient_colour");
			glUniform3f(matAmbientLocation, material.getAmbientColour().x, material.getAmbientColour().y, material.getAmbientColour().z);
			GLuint matDiffuseLocation = glGetUniformLocation(shader_program_, "mat.diffuse_colour");
			glUniform3f(matDiffuseLocation, material.getDiffuseColour().x, material.getDiffuseColour().y, material.getDiffuseColour().z);
			GLuint matSpecularLocation = glGetUniformLocation(shader_program_, "mat.specular_colour");
			glUniform3f(matSpecularLocation, material.getSpecularColour().x, material.getSpecularColour().y, material.getSpecularColour().z);
			GLuint matShininessLocation = glGetUniformLocation(shader_program_, "mat.shininess");
			glUniform1f(matShininessLocation, material.getShininess());

			//reset bound textures by setting them to 0
			glActiveTexture(GL_TEXTURE0 + kDiffuseTexture);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0 + kSpecularTexture);
			glBindTexture(GL_TEXTURE_2D, 0);

			GLuint matHasTexture = glGetUniformLocation(shader_program_, "mat.hasDiffuse");
			//check if material has a diffuse texture
			if (!material.getDiffuseTexture().empty())
			{
				//bind diffuse texture
				glActiveTexture(GL_TEXTURE0 + kDiffuseTexture);
				glBindTexture(GL_TEXTURE_2D, m_textures[material.getDiffuseTexture()]);
				glUniform1f(matHasTexture, true);
			}
			else
			{
				//no diffuse
				glUniform1f(matHasTexture, false);
			}

			//check if material has a specular texture
			matHasTexture = glGetUniformLocation(shader_program_, "mat.hasSpecular");
			if (!material.getSpecularTexture().empty())
			{
				//bind specular texture
				glActiveTexture(GL_TEXTURE0 + kSpecularTexture);
				glBindTexture(GL_TEXTURE_2D, m_textures[material.getSpecularTexture()]);
				glUniform1f(matHasTexture, true);
			}
			else
			{
				//no specular texture
				glUniform1f(matHasTexture, false);
			}

			GLuint diffuse_sampler_id = glGetUniformLocation(shader_program_, "mat.diffuse_sampler");
			glUniform1i(diffuse_sampler_id, kDiffuseTexture);
			GLuint specular_sampler_id = glGetUniformLocation(shader_program_, "mat.specular_sampler");
			glUniform1i(specular_sampler_id, kSpecularTexture);


			// Finally you render the mesh e.g.
			glBindVertexArray(mesh.vao);
			glDrawElements(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, 0);
		}
	}
}

void MyView::buildMesh(Mesh & mesh,int meshID, std::vector<Vertex> vertices, std::vector<unsigned int> elements)
{
	//set mesh id
	mesh.mesh_id = meshID;

	glGenBuffers(1, &mesh.vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(Vertex), // size of data in bytes
		vertices.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, kNullId);


	glGenBuffers(1, &mesh.element_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.element_vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		elements.size() * sizeof(unsigned int),
		elements.data(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, kNullId);
	mesh.element_count = elements.size();

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.element_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_vbo);
	glEnableVertexAttribArray(kVertexPosition);
	glVertexAttribPointer(kVertexPosition, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)offsetof(Vertex, Vertex::position));
	glEnableVertexAttribArray(kVertexNormal);
	glVertexAttribPointer(kVertexNormal, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)offsetof(Vertex, Vertex::normal));
	glEnableVertexAttribArray(kVertexUV);
	glVertexAttribPointer(kVertexUV, 2, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)offsetof(Vertex, Vertex::texCoord));

	glBindBuffer(GL_ARRAY_BUFFER, kNullId);
	glBindVertexArray(kNullId);
}

void MyView::createTexture(const std::string & path, GLuint & texID)
{
	tygra::Image texture_image
		= tygra::createImageFromPngFile(path);
	if (texture_image.doesContainData()) {
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			texture_image.width(),
			texture_image.height(),
			0,
			pixel_formats[texture_image.componentsPerPixel()],
			texture_image.bytesPerComponent() == 1
			? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
			texture_image.pixelData());
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, kNullId);
	}
}