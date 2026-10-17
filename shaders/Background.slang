struct Constants {
  float4 topColor;
  float4 bottomColor;
};

[[vk::push_constant]] Constants constants;

struct VertexOutput {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};

// One triangle that covers the whole screen.
[shader("vertex")]
VertexOutput vsMain(uint vertexId: SV_VulkanVertexID) {
  float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
  VertexOutput output;
  output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
  output.uv = uv;
  return output;
}

[shader("fragment")]
float4 fsMain(VertexOutput input) : SV_Target {
  return lerp(constants.topColor, constants.bottomColor, input.uv.y);
}
