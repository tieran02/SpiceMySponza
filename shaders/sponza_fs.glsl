#version 330

struct Light
{
	vec3 position;
	vec3 intensity;
	vec3 direction;
	float range;
};

struct Material 
{
	vec3 ambient_colour;
	vec3 diffuse_colour;
	vec3 specular_colour;
	float shininess;

	bool hasDiffuse;
	bool hasSpecular;
	sampler2D diffuse_sampler;
	sampler2D specular_sampler;
};

out vec4 fragment_colour;

in vec3 vNormal;
in vec3 FragPos;
in vec2 UV;

uniform Light Lights[24];
uniform Material mat;
uniform vec3 cameraPos;
uniform vec3 ambientIntensityColour;

//Specular phone function gets the specular colour
//The reason it is abstracted is because multiple light casters need the specular
vec3 SpecularPhong(Material mat, vec3 L, vec3 N)
{
	//Calculate specular colour
	if (mat.shininess > 0)
	{
		vec3 viewDir = normalize(cameraPos - FragPos);
		vec3 reflectDir = reflect(-L, N);
		float spec_intensity = pow(max(0.0, dot(viewDir, reflectDir)), mat.shininess);

		// If the material has a specular texture then multiply the spec colour by the spec texture
		if (mat.hasSpecular) 
		{
			vec4 specularTexture = texture(mat.specular_sampler, UV);
			return mat.specular_colour * spec_intensity * specularTexture.rgb;
		}
		//else return the spec colour
		return mat.specular_colour * spec_intensity;
	}
	//If the material has no shininess then return a vec3 Zero.
	return vec3(0,0,0);
}

vec3 PointLight(Light light, vec3 N, vec3 SurfaceColour)
{
	vec3 L = normalize(light.position - FragPos);

	//Calculate diffuse colour
	float diffuse_intensity = max(0.0, dot(L, N));
	vec3 diffusePhong = SurfaceColour * diffuse_intensity;

	vec3 specularPhong = SpecularPhong(mat, L, N);

	float lightDistance = distance(light.position, FragPos);
	float attenuation = smoothstep(light.range, light.range/2, lightDistance);

	return vec3(light.intensity * (diffusePhong + specularPhong)) * attenuation;
}

vec3 SpotLight(Light light, vec3 N, vec3 SurfaceColour)
{
	vec3 L = normalize(light.position - FragPos);

	//Calculate diffuse colour
	float diffuse_intensity = max(0.0, dot(L, N));
	vec3 diffusePhong = SurfaceColour * diffuse_intensity;
	//Calculate specular
	vec3 specularPhong = SpecularPhong(mat, L, N);

	float spot = dot(normalize(light.direction), normalize(-L));

	float cutOff = cos(radians(light.range));
	if (spot < cutOff)
	{
		spot = smoothstep(cutOff- 0.02, cutOff, spot);
	}
	else spot = 1.0;
	
	return vec3(light.intensity * (diffusePhong + specularPhong)) * spot;
}

vec3 DirectionalLight(Light light, vec3 N, vec3 SurfaceColour)
{
	vec3 L = normalize(light.position - FragPos);
	vec3 lightDir = normalize(-light.direction);

	//Calculate diffuse colour
	float diffuse_intensity = max(0.0, dot(lightDir, N));
	vec3 diffusePhong = SurfaceColour * diffuse_intensity;
	//Calculate specular
	vec3 specularPhong = SpecularPhong(mat, L, N);

	return vec3(light.intensity * (diffusePhong + specularPhong));
}

void main(void)
{
	vec4 diffuseTexture = texture(mat.diffuse_sampler, UV);

	vec3 SurfaceColour;
	if (mat.hasDiffuse)
		SurfaceColour = mat.diffuse_colour * diffuseTexture.rgb;
	else
		SurfaceColour = mat.diffuse_colour;

	//Normalise varying normal
	vec3 N = normalize(vNormal);

	//ambient light 
	vec3 ambientLight = (ambientIntensityColour * mat.ambient_colour * SurfaceColour);
	vec3 finalColour = ambientLight;

	for (int i = 0; i < 24; i++)
	{
		if (i == 23)
			finalColour += DirectionalLight(Lights[i], N, SurfaceColour);
		if (i == 22)
			finalColour += SpotLight(Lights[i], N, SurfaceColour);
		else
		{
			finalColour += PointLight(Lights[i], N, SurfaceColour);
		}
	}

	fragment_colour = vec4(finalColour, 1.0);
}
