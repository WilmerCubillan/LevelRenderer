// TODO: 2i
#pragma pack_matrix(row_major)
// an ultra simple hlsl vertex shader
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
    uint illum; // illumination model
};

struct SHADER_MODEL_DATA
{

    float4 sunDirection, sunColor; // Lighting info
    matrix viewMatrix, projectionMatrix; // Viewing
	// Per sub-mesh transform and material data
    matrix matricies[MAX_SUBMESH_PER_DRAW];
    OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
};


// TODO: Part 4g
// TODO: Part 2i
StructuredBuffer<SHADER_MODEL_DATA> SceneData;

// TODO: Part 3e
[[vk::push_constant]]
cbuffer MESH_INDEX
{
    uint mesh_ID;
};
// TODO: Part 4a
struct OUTPUT_TO_RASTERIZER
{
    float4 posH : SV_POSITION; // Homogeneous projection space
    float3 nrmW : NORMAL; // normal in world space (for lighting)
    float3 posW : WORLD; // position in world space (for lighting)
};
// TODO: Part 1f
struct VS_INPUT {

	float4 pos : POSITION; // Left-handed +Z forward coordinate w not provided, assumed to be 1.
	float4 uvw : TEXCOORD; // D3D/Vulkan style top left 0,0 coordinate.
	float4 nrm : NORMAL; // Provided direct from obj file, may or may not be normalized.

};

struct VS_OUTPUT {

	float4 pos : SV_POSITION;
    float4 uvw : TEXCOORD;
    float4 nrm : NORMAL;
	
};
// TODO: Part 4b
OUTPUT_TO_RASTERIZER main(VS_INPUT inputVertex)
{
    OUTPUT_TO_RASTERIZER output = (OUTPUT_TO_RASTERIZER)0;
	// TODO: Part 1h
    output.posH = inputVertex.pos;
    //output.uvw = inputVertex.uvw;
    output.nrmW = inputVertex.nrm;
	
	//output.posH.z += 0.5f;
	output.posH.y += -0.95f;
	// TODO: Part 2i
    output.posH = mul(output.posH, SceneData[0].matricies[mesh_ID]);
    output.posH = mul(output.posH, SceneData[0].viewMatrix);
    output.posH = mul(output.posH, SceneData[0].projectionMatrix);
	// TODO: Part 4e
	// TODO: Part 4b
    output.nrmW = mul(output.nrmW, SceneData[0].matricies[mesh_ID]);
	// TODO: Part 4e
	return output;
}