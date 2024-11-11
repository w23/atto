vec3 fragMain(vec2 uv, float t) {
  return vec3(sin(length(uv)*7. - t * 3.));
}
