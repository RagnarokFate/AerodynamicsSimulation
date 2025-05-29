#version 330 core

struct Material
{
	sampler2D textureMap;
	sampler2D normalMap;  

	// You can add more fields here...
	// Such as:
	//		1. diffuse/specular relections constants
	//		2. specular alpha constant
	//		3. Anything else you find relevant
};

// We set this field's properties from the C++ code
uniform Material material;

//Type of coloring
uniform int TriangleFillMode;
uniform int MappingMode; //internal of Mesh Model - Default, PlaneMapping, CylinderMapping, SphereMapping
uniform int MappingExtra; //"outsider" of Mesh Model - NoMapping ,NormalMapping, EnvironmentMapping, ToonShading
uniform vec3 MeshModelColor;

//Light Properties
uniform vec3 LightPosition;
uniform vec3 LightAmbient;
uniform vec3 LightDiffuse;
uniform vec3 LightSpecular;
uniform float LightSpecularDegree;
uniform float LightIntensity;
uniform vec3 LightSobel;


//MeshModel Properties
uniform vec3 MeshModelAmbient;
uniform	vec3 MeshModelDiffuse;
uniform	vec3 MeshModelSpecular;

//Camera View Properties
uniform vec3 CameraPosition;

// Inputs from vertex shader (after interpolation was applied)
in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoords;
in vec3 orig_fragPos;
// The final color of the fragment (pixel)
out vec4 frag_color;

void main()
{
	if(MappingExtra == 0)
	{
	// Sample the texture-map at the UV coordinates given by 'fragTexCoords'
	//vec3 textureColor = vec3(texture(material.textureMap, fragTexCoords));
	//frag_color = vec4(orig_fragPos,1);
		switch(TriangleFillMode)
		{
			case 0: // Normal
			frag_color = vec4(orig_fragPos,1.0f);
				break;
	//=====================================================================================================
			case 1: //Specfic Color
			frag_color = vec4(MeshModelColor,1.0f);
				break;
	//=====================================================================================================
			case 2: //With Light No Texture
				vec3 ColorPixel = vec3(0,0,0);
				//Ambient Calculation
				vec3 Ambient = LightAmbient * MeshModelAmbient;
				//Diffuse Calculation
				vec3 norm = normalize(fragNormal);
				vec3 lightDir = normalize(LightPosition - fragPos);  
				float DiffuseAngle = max(dot(norm, lightDir), 0.0);
				vec3 Diffuse = DiffuseAngle * LightDiffuse * MeshModelDiffuse;
				//Specular Calculation
				vec3 viewDir = normalize(CameraPosition - fragPos);
				vec3 reflectDir = reflect(-lightDir, norm);  
				float SpecularAngle = pow(max(dot(viewDir, reflectDir), 0.0), LightSpecularDegree);
				vec3 Specular = SpecularAngle * LightSpecular * MeshModelSpecular;
				ColorPixel = Ambient + Diffuse + Specular;
				frag_color = vec4(ColorPixel,1);
				break;
	//=====================================================================================================
			case 3: //With Texture No Light
				vec3 TextureColor = vec3(texture(material.textureMap, fragTexCoords));
				frag_color = vec4(TextureColor,1.0f);
				break;
	//=====================================================================================================
			case 4: //With Light Texture
				vec3 ColorPixelLT = vec3(0.0f,0.0f,0.0f);
				vec3 AmbientLT = LightAmbient * MeshModelAmbient;
				//Diffuse Calculation
				vec3 normLT  = normalize(fragNormal);
				vec3 lightDirLT  = normalize(LightPosition - fragPos);  
				float DiffuseAngleLT  = max(dot(normLT , lightDirLT ), 0.0);
				vec3 DiffuseLT  = DiffuseAngleLT  * LightDiffuse * MeshModelDiffuse;
				//Specular Calculation
				vec3 viewDirLT  = normalize(CameraPosition - fragPos);
				vec3 reflectDirLT  = reflect(-lightDirLT , normLT );  
				float SpecularAngleLT  = pow(max(dot(viewDirLT , reflectDirLT), 0.0), LightSpecularDegree);
				vec3 SpecularLT  = SpecularAngleLT  * LightSpecular * MeshModelSpecular;
				ColorPixelLT  = Ambient + Diffuse + Specular;
				vec3 TextureColorLT  = vec3(texture(material.textureMap, fragTexCoords));
				ColorPixelLT  = ColorPixelLT + TextureColorLT;
				frag_color = vec4(ColorPixelLT,1.0f);
				break;
	//=====================================================================================================
			default:
				break;
	
		}
	}
	else if(MappingExtra == 1) //Normal Mapping...
	{
		vec3 normal = vec3(texture(material.normalMap, fragTexCoords));
		//transform normal vector to range [-1,1]
		normal = normalize(normal * 2.0 - 1.0);

		//Lighting Calculation
		{
			vec3 ColorPixelNM = vec3(0,0,0);
			//Ambient Calculation
			vec3 AmbientNM = LightAmbient * MeshModelAmbient;
			//Diffuse Calculation
			vec3 normNM = normalize(normal);
			vec3 lightDirNM = normalize(LightPosition - fragPos);  
			float DiffuseAngleNM = max(dot(normNM, lightDirNM), 0.0);
			vec3 DiffuseNM = DiffuseAngleNM * LightDiffuse * MeshModelDiffuse;
			//Specular Calculation
			vec3 viewDirNM = normalize(CameraPosition - fragPos);
			vec3 reflectDirNM = reflect(-lightDirNM, normNM);  
			float SpecularAngleNM = pow(max(dot(viewDirNM, reflectDirNM), 0.0), LightSpecularDegree);
			vec3 SpecularNM = SpecularAngleNM * LightSpecular * MeshModelSpecular;
			ColorPixelNM = AmbientNM + DiffuseNM + SpecularNM;
			frag_color = vec4(ColorPixelNM,1);
		}

	}
	else if(MappingExtra == 3) //Toom Shade
	{
		// Step 1: Calculate the lighting intensity
		float intensity = LightIntensity;

		//Caluclation of Light Pixel
		vec3 ColorPixelTS = vec3(0,0,0);
		{
			//Ambient Calculation
			vec3 AmbientTS = LightAmbient * MeshModelAmbient;
			//Diffuse Calculation
			vec3 normTS = normalize(fragNormal);
			vec3 lightDirTS = normalize(LightPosition - fragPos);  
			float DiffuseAngleTS = max(dot(normTS, lightDirTS), 0.0);
			vec3 DiffuseTS = DiffuseAngleTS * LightDiffuse * MeshModelDiffuse;
			//Specular Calculation
			vec3 viewDirTS = normalize(CameraPosition - fragPos);
			vec3 reflectDirTS = reflect(-lightDirTS, normTS);  
			float SpecularAngleTS = pow(max(dot(viewDirTS, reflectDirTS), 0.0), LightSpecularDegree);
			vec3 SpecularTS = SpecularAngleTS * LightSpecular * MeshModelSpecular;
			ColorPixelTS = AmbientTS + DiffuseTS + SpecularTS;
		}
		// Step 2: Quantize the intensity
		float quantized = floor(intensity * 4.0) / 4.0;

		// Step 3: Detect edges
		vec3 sobel = LightSobel;

		// Step 4: Apply the toon shading effect
		vec3 color = vec3(texture(material.textureMap, fragTexCoords));
		vec3 finalColor = mix(color, vec3(0.0), sobel.r);
		finalColor = mix(finalColor, vec3(0.5), sobel.g);
		finalColor = mix(finalColor, vec3(1.0), sobel.b);
		finalColor = mix(finalColor, vec3(quantized), 0.5);
		finalColor = mix(finalColor,ColorPixelTS,0.5);
		frag_color = vec4(finalColor, 1.0);
	}
}
