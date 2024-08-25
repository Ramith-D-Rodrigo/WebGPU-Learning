// Variable in the *uniform* address space
// The memory location of the uniform is given by a pair of a *bind group* and
// a *binding*.
@group(0) @binding(0) var<uniform> uTime: f32;

struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    // The location here does not refer to a vertex attribute, it just means
    // that this field must be handled by the rasterizer.
    // (It can also refer to another field of another struct that would be used
    // as input to the fragment shader.)
    @location(0) color: vec3f,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var offset = vec2f(-0.6875, -0.463);
    offset += 0.3 * vec2f(cos(uTime), sin(uTime));

    let ratio = 800.0 / 600.0;
    var out: VertexOutput;
    out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}