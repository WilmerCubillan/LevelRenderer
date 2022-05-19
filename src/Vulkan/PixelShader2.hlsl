// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024
struct OBJ_ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
	unsigned int illum; // illumination model
};

struct VS_INPUT
{
    float4 pos : POSITION; // Left-handed +Z forward coordinate w not provided, assumed to be 1.
    float4 uvw : TEXCOORD; // D3D/Vulkan style top left 0,0 coordinate.
    float4 nrm : NORMAL; // Provided direct from obj file, may or may not be normalized.
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 uvw : TEXCOORD;
    float4 nrm : NORMAL;
};

struct SHADER_MODEL_DATA
{

    float4 sunDirection, sunColor; // Lighting info
    matrix viewMatrix, projectionMatrix; // Viewing
		// Per sub-mesh transform and material data
    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
};

StructuredBuffer<SHADER_MODEL_DATA> SceneData;
// TODO: Part 4g
// TODO: Part 2i

// TODO: Part 3e
// an ultra simple hlsl pixel shader
// TODO: Part 4b
float4 main(PS_INPUT input) : SV_TARGET
{
    float4 finalColor = 0;
    finalColor = SceneData[0].materials[0];
    finalColor.a = 1.0f;
    
    return float4(0.75f, 0.75f, 0.75f, 0);
	// TODO: Part 3a
	// TODO: Part 4c
	// TODO: Part 4g (half-vector or reflect method your choice)
}
