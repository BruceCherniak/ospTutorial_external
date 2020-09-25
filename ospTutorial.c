// Copyright 2020 Jefferson Amstutz
// SPDX-License-Identifier: MIT

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
// ospray
#include "ospray/ospray_util.h"
// stb
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, const char **argv) {
  stbi_flip_vertically_on_write(1);

  int imgSize_x = 1024;
  int imgSize_y = 768;

  float cam_pos[] = {0.f, 0.f, 0.f};
  float cam_up[] = {0.f, 1.f, 0.f};
  float cam_view[] = {0.1f, 0.f, 1.f};

  float vertex[] = {-1.0f, -1.0f, 3.0f, -1.0f, 1.0f, 3.0f,
                    1.0f,  -1.0f, 3.0f, 0.1f,  0.1f, 0.3f};
  float color[] = {0.9f, 0.5f, 0.5f, 1.0f, 0.8f, 0.8f, 0.8f, 1.0f,
                   0.8f, 0.8f, 0.8f, 1.0f, 0.5f, 0.9f, 0.5f, 1.0f};
  int32_t index[] = {0, 1, 2, 1, 2, 3};

  printf("initialize OSPRay...");

  OSPError init_error = ospInit(&argc, argv);
  if (init_error != OSP_NO_ERROR)
    return init_error;

  printf("done\n");
  printf("setting up camera...");

  OSPCamera camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", imgSize_x / (float)imgSize_y);
  ospSetParam(camera, "position", OSP_VEC3F, cam_pos);
  ospSetParam(camera, "direction", OSP_VEC3F, cam_view);
  ospSetParam(camera, "up", OSP_VEC3F, cam_up);
  ospCommit(camera);

  printf("done\n");
  printf("setting up scene...");

  OSPGeometry mesh = ospNewGeometry("mesh");

  OSPData data = ospNewSharedData1D(vertex, OSP_VEC3F, 4);

  ospCommit(data);
  ospSetObject(mesh, "vertex.position", data);
  ospRelease(data);

  data = ospNewSharedData1D(color, OSP_VEC4F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.color", data);
  ospRelease(data);

  data = ospNewSharedData1D(index, OSP_VEC3UI, 2);
  ospCommit(data);
  ospSetObject(mesh, "index", data);
  ospRelease(data);

  ospCommit(mesh);

  OSPMaterial mat = ospNewMaterial("pathtracer", "obj");
  ospCommit(mat);

  OSPGeometricModel model = ospNewGeometricModel(mesh);
  ospSetObject(model, "material", mat);
  ospCommit(model);
  ospRelease(mesh);
  ospRelease(mat);

  OSPGroup group = ospNewGroup();
  ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, model);
  ospCommit(group);
  ospRelease(model);

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  OSPWorld world = ospNewWorld();
  ospSetObjectAsData(world, "instance", OSP_INSTANCE, instance);
  ospRelease(instance);

  OSPLight light = ospNewLight("ambient");
  ospCommit(light);
  ospSetObjectAsData(world, "light", OSP_LIGHT, light);
  ospRelease(light);

  ospCommit(world);

  printf("done\n");

  OSPBounds worldBounds = ospGetBounds(world);
  printf("world bounds: ({%f, %f, %f}, {%f, %f, %f}\n\n", worldBounds.lower[0],
         worldBounds.lower[1], worldBounds.lower[2], worldBounds.upper[0],
         worldBounds.upper[1], worldBounds.upper[2]);

  printf("setting up renderer...");

  OSPRenderer renderer = ospNewRenderer("pathtracer");

  ospSetFloat(renderer, "backgroundColor", 1.0f);
  ospCommit(renderer);

  OSPFrameBuffer framebuffer = ospNewFrameBuffer(
      imgSize_x, imgSize_y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
  ospResetAccumulation(framebuffer);

  printf("rendering initial frame to firstFrame.png...");

  ospRenderFrameBlocking(framebuffer, renderer, camera, world);

  const uint32_t *fb = (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  stbi_write_png("firstFrame.png", imgSize_x, imgSize_y, 4, fb, 4 * imgSize_x);
  ospUnmapFrameBuffer(fb, framebuffer);

  printf("done\n");
  printf("rendering 10 accumulated frames to accumulatedFrame.png...");

  for (int frames = 0; frames < 10; frames++)
    ospRenderFrameBlocking(framebuffer, renderer, camera, world);

  fb = (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  stbi_write_png("accumulatedFrame.png", imgSize_x, imgSize_y, 4, fb,
                 4 * imgSize_x);
  ospUnmapFrameBuffer(fb, framebuffer);

  printf("done\n\n");

  OSPPickResult p;
  ospPick(&p, framebuffer, renderer, camera, world, 0.5f, 0.5f);

  printf("ospPick() center of screen --> [inst: %p, model: %p, prim: %u]\n",
         p.instance, p.model, p.primID);

  printf("cleaning up objects...");

  ospRelease(p.instance);
  ospRelease(p.model);

  ospRelease(renderer);
  ospRelease(camera);
  ospRelease(framebuffer);
  ospRelease(world);

  printf("done\n");

  ospShutdown();

  return 0;
}
