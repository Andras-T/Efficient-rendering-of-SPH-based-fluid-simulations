struct Constants {
  uint* values;
  uint count;
};

[[vk::push_constant]] Constants constants;

[shader("compute")]
[numthreads(64, 1, 1)]
void csMain(uint3 threadId: SV_DispatchThreadID) {
  if (threadId.x >= constants.count) {
    return;
  }
  constants.values[threadId.x] = constants.values[threadId.x] * 2 + 1;
}
