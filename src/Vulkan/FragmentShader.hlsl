// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024
struct SHADER_OBJ_ATTRIBUTES
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
SHADER_OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];

};

StructuredBuffer<SHADER_MODEL_DATA> SceneData;

// TODO: Part 4g
// TODO: Part 2i

// TODO: Part 3e
[[vk::push_constant]]
cbuffer MESH_INDEX
{
    uint mesh_ID;
};
// an ultra simple hlsl pixel shader
// TODO: Part 4b
struct OUTPUT_TO_RASTERIZER
{
    float4 posH : SV_POSITION; // Homogeneous projection space
    float3 nrmW : NORMAL; // normal in world space (for lighting)
    float3 posW : WORLD; // position in world space (for lighting)
};

float4 main(OUTPUT_TO_RASTERIZER output) : SV_TARGET
{
    float4 finalColor = 0;
    finalColor.x = SceneData[0].materials[mesh_ID].Kd.x;
    finalColor.y = SceneData[0].materials[mesh_ID].Kd.y;
    finalColor.z = SceneData[0].materials[mesh_ID].Kd.z;
   
    finalColor -= saturate(dot((float3) -SceneData[0].sunDirection[mesh_ID], output.nrmW) * SceneData[0].sunColor[mesh_ID]);
    
    finalColor.a = 1.0f;
    return finalColor;
}





//float4 main() : SV_TARGET
//{
//    return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}