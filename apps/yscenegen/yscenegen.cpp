//
// LICENSE:
//
// Copyright (c) 2016 -- 2020 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <yocto/yocto_commonio.h>
#include <yocto/yocto_image.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
using namespace yocto::math;
namespace sio = yocto::sceneio;
namespace shp = yocto::shape;
namespace cli = yocto::commonio;

#include <memory>
using std::string;
using namespace std::string_literals;

#include "ext/filesystem.hpp"
namespace sfs = ghc::filesystem;

#include "ext/perlin-noise/noise1234.h"

float noise(const vec3f& p) { return noise3(p.x, p.y, p.z); }
vec2f noise2(const vec3f& p) {
  return {noise(p + vec3f{0, 0, 0}), noise(p + vec3f{3, 7, 11})};
}
vec3f noise3(const vec3f& p) {
  return {noise(p + vec3f{0, 0, 0}), noise(p + vec3f{3, 7, 11}),
      noise(p + vec3f{13, 17, 19})};
}
float fbm(const vec3f& p, int octaves) { 
    // YOUR CODE GOES HERE ------------------------------
    float fbm = 0.f;
    // sommatoria
    for (int i = 0; i <= octaves; i++)
        fbm += pow(2, -i) * noise(pow(2, i) * p);
    return fbm;
}

float turbulence(const vec3f& p, int octaves) { 
    // YOUR CODE GOES HERE ------------------------------
    float turb = 0.f;
    //sommatoria
    for (int i = 0; i <= octaves; i++)
        turb += pow(2.f, -i) * yocto::math::abs(noise(yocto::math::pow(2, i) * p));
    return turb;
}
float ridge(const vec3f& p, int octaves) {
    // YOUR CODE GOES HERE ------------------------------
    float ridge = 0.f;
    //sommatoria
    for (int i = 0; i <= octaves; i++)
        ridge += pow(2.f, -i) * yocto::math::pow(1.f - yocto::math::abs(noise(pow(2.f, i) * p)), 2.f) / 2.f;
    return ridge;
}

sio::object* get_object(sio::model* scene, const std::string& name) {
  for (auto object : scene->objects)
    if (object->name == name) return object;
  cli::print_fatal("unknown object " + name);
  return nullptr;
}

void add_polyline(sio::shape* shape, const std::vector<vec3f>& positions,
    const std::vector<vec3f>& colors, float thickness = 0.0001f) {
  auto offset = (int)shape->positions.size();
  shape->positions.insert(
      shape->positions.end(), positions.begin(), positions.end());
  shape->colors.insert(shape->colors.end(), colors.begin(), colors.end());
  shape->radius.insert(shape->radius.end(), positions.size(), thickness);
  for (auto idx = 0; idx < positions.size() - 1; idx++) {
    shape->lines.push_back({offset + idx, offset + idx + 1});
  }
}

void sample_shape(std::vector<vec3f>& positions, std::vector<vec3f>& normals,
    std::vector<vec2f>& texcoords, sio::shape* shape, int num) {
  auto triangles  = shape->triangles;
  auto qtriangles = shp::quads_to_triangles(shape->quads);
  triangles.insert(triangles.end(), qtriangles.begin(), qtriangles.end());
  auto cdf = shp::sample_triangles_cdf(triangles, shape->positions);
  auto rng = make_rng(19873991);
  for (auto idx = 0; idx < num; idx++) {
    auto [elem, uv] = shp::sample_triangles(cdf, rand1f(rng), rand2f(rng));
    auto q          = triangles[elem];
    positions.push_back(interpolate_triangle(shape->positions[q.x],
        shape->positions[q.y], shape->positions[q.z], uv));
    normals.push_back(normalize(interpolate_triangle(
        shape->normals[q.x], shape->normals[q.y], shape->normals[q.z], uv)));
    if (!texcoords.empty()) {
      texcoords.push_back(interpolate_triangle(shape->texcoords[q.x],
          shape->texcoords[q.y], shape->texcoords[q.z], uv));
    } else {
      texcoords.push_back(uv);
    }
  }
}

struct terrain_params {
  float size    = 0.1f;
  vec3f center  = zero3f;
  float height  = 0.1f;
  float scale   = 10;
  int   octaves = 8;
  vec3f bottom  = srgb_to_rgb(vec3f{154, 205, 50} / 255);
  vec3f middle  = srgb_to_rgb(vec3f{205, 133, 63} / 255);
  vec3f top     = srgb_to_rgb(vec3f{240, 255, 255} / 255);
};

void make_terrain(
    sio::model* scene, sio::object* object, const terrain_params& params) {
    // YOUR CODE GOES HERE ------------------------------
    // Scorro tra i vertici
    for (int i=0; i<object->shape->positions.size(); i++)
    {
        // Prendo la posizione del vertice (lo salvo in questa variabile per rendere più leggibile il codice)
        vec3f pos = object->shape->positions[i];

        // calcolo la nuova posizione, attraverso 
        object->shape->positions[i] += params.height * ridge(pos * params.scale, params.octaves)
            * (1.f - yocto::math::length(pos - params.center) / params.size)
            * object->shape->normals[i]; //muovo lungo la normale

        // trasformo l'altezza del vertice in percentuale
        auto height = object->shape->positions[i].y / params.height * 100;
        // colore in base alla percentuale
        object->shape->colors.push_back(height <= 30 ? params.bottom : height <= 60 ? params.middle : params.top);
    }
    // Aggiorno le normali
    yocto::shape::update_normals(object->shape->normals, object->shape->quads, object->shape->positions);
}

struct displacement_params {
  float height = 0.02f;
  float scale  = 50;
  int   octaves = 8;
  vec3f bottom = srgb_to_rgb(vec3f{64, 224, 208} / 255);
  vec3f top    = srgb_to_rgb(vec3f{244, 164, 96} / 255);
};

void make_displacement(
    sio::model* scene, sio::object* object, const displacement_params& params) {
    // Scorro tra i vertici
    for (int i = 0; i < object->shape->positions.size(); i++)
    {
        // Mantengo in una variabile la posizione di partenza (la userò per il calcolo dell'altezza)
        vec3f pos = object->shape->positions[i];
        // Calcolo la nuova posizione del vertice usando turbulence e lo muovo lungo la normale
        object->shape->positions[i] += object->shape->normals[i] * params.height * turbulence(pos * params.scale, params.octaves);
        
        // Prendo la distanza dal punto di inizio e il punto appena calcolato e divido per l'altezza massima
        // ottenendo un numero tra [0,1], così da poterlo usare nell'interpolazione
        auto height = yocto::math::distance(object->shape->positions[i], pos) / params.height;
        
        // si potrebbe sostituire con interpolate_line
        auto c = params.bottom * (1 - height) + params.top * (height);
        // salvo il colore
        object->shape->colors.push_back(c);
    }

    yocto::shape::update_normals(object->shape->normals, object->shape->quads, object->shape->positions);
    
}

struct hair_params {
  int   num      = 100000;
  int   steps    = 1;
  float lenght   = 0.02f;
  float scale    = 250;
  float strength = 0.01f;
  float gravity  = 0.0f;
  vec3f bottom   = srgb_to_rgb(vec3f{25, 25, 25} / 255);
  vec3f top      = srgb_to_rgb(vec3f{244, 164, 96} / 255);
};

void make_hair(sio::model* scene, sio::object* object, sio::object* hair,
    const hair_params& params) {
  // YOUR CODE GOES HERE ------------------------------
    // Mantengo in una variabile quanti vertici ho in positions
    int old_obj_size = object->shape->positions.size();
    // Aggiungo una shape alla scena
    hair->shape = add_shape(scene);
    // Sample shape su object così da avere in modo uniforme dei punti su cui "inserire" i capelli
    sample_shape(object->shape->positions, object->shape->normals, object->shape->texcoords, object->shape, params.num);
    
    // Scorro sui nuovi vertici appena aggiunti
    for (int i = old_obj_size; i < object->shape->positions.size(); i++)
    {
        // Mi creo due vettori per ogni linea, uno per i vertici e uno per i colori
        std::vector<vec3f> positions;
        std::vector<vec3f> colors;
        // Prendo la posizione del vertice "iniziale" del capello
        vec3f pos = object->shape->positions[i];
        // Lo aggiungo alle posizioni, e setto il suo colore a params.bottom
        positions.push_back(pos);
        colors.push_back(params.bottom);
        // Inizializzo questa variabile che mi dà la direzione
        auto normale = object->shape->normals[i];
        for (int step = 0; step < params.steps; step++)
        {
            // Mi mantengo la vecchia posizione
            auto old_pos = pos;
            //calcolo la nuova posizione
            pos += normale * (params.lenght / params.steps) + noise3(pos * params.scale) * params.strength;
            // applico la gravità
            pos.y -= params.gravity;

            // Calcolo la nuova direzione per il prossimo step
            normale = normalize(pos - old_pos);
            // Aggiungo posizione e colore ai due array
            positions.push_back(pos);
            // colore ottenuto interpolando
            colors.push_back(yocto::math::interpolate_line(
                params.bottom, params.top, distance(pos, object->shape->positions[i])/params.lenght
            ));
        }
        // Questo mi fixa un bug sui colori
        colors[colors.size() - 1] = params.top;
        add_polyline(hair->shape, positions, colors);
    }
    
    for (auto tangent : yocto::shape::compute_tangents(hair->shape->lines, hair->shape->positions))
        hair->shape->tangents.push_back(vec4f{ tangent.x, tangent.y, tangent.z, 1 });
}

struct grass_params {
  int num = 10000;
};

float map(float in, float is, float ie, float os, float oe)
{
    return os + ((oe - os) / (ie - is)) * (in - is);
}

void make_grass(sio::model* scene, sio::object* object,
    const std::vector<sio::object*>& grasses, const grass_params& params) {
    auto rng = make_rng(198767);
    for (auto grass : grasses) grass->instance = add_instance(scene);
    // YOUR CODE GOES HERE ------------------------------
    sample_shape(object->shape->positions, object->shape->normals, object->shape->texcoords, object->shape, params.num);
    for (int i = 0; i < object->shape->positions.size(); i++)
    {
        // Prendo un'istanza a caso
        // aggiungo alla scena un oggetto con i dati dell'istanza
        int i1 = rand1i(rng, grasses.size());
        sio::object* grass_obj = add_object(scene);
        grass_obj->shape = grasses[i1]->shape;
        grass_obj->material = grasses[i1]->material;

        // Mi creo i vari valori random per cui scalare e ruotare
        // MAP: è una funzione che dato un range di possibili input mappa in un range di output
        auto randScal = map(rand1f(rng), 0.f, .9999f, 0.9f, 1.f);
        auto randRotZ = map(rand1f(rng), 0.f, .9999f, 0.1f, 0.2f);
        auto randRotY = map(rand1f(rng), 0.f, .9999f, 0.f, 2.f * yocto::math::pi);

        // Setto il frame con origine nella posizione attuale
        grass_obj->frame = translation_frame(object->shape->positions[i]);
        // Scalo e ruoto
        grass_obj->frame *= scaling_frame(vec3f(randScal)) 
            * rotation_frame(grass_obj->frame.y, randRotY)
            * rotation_frame(grass_obj->frame.z, randRotZ);
    }
}

void make_dir(const std::string& dirname) {
  if (sfs::exists(dirname)) return;
  try {
    sfs::create_directories(dirname);
  } catch (...) {
    cli::print_fatal("cannot create directory " + dirname);
  }
}

int main(int argc, const char* argv[]) {
  // command line parameters
  auto terrain      = ""s;
  auto tparams      = terrain_params{};
  auto displacement = ""s;
  auto dparams      = displacement_params{};
  auto hair         = ""s;
  auto hairbase     = ""s;
  auto hparams      = hair_params{};
  auto grass        = ""s;
  auto grassbase    = ""s;
  auto gparams      = grass_params{};
  auto output       = "out.json"s;
  auto filename     = "scene.json"s;

  // parse command line
  auto cli = cli::make_cli("yscenegen", "Make procedural scenes");
  add_option(cli, "--terrain", terrain, "terrain object");
  add_option(cli, "--displacement", displacement, "displacement object");
  add_option(cli, "--hair", hair, "hair object");
  add_option(cli, "--hairbase", hairbase, "hairbase object");
  add_option(cli, "--grass", grass, "grass object");
  add_option(cli, "--grassbase", grassbase, "grassbase object");
  add_option(cli, "--hairnum", hparams.num, "hair number");
  add_option(cli, "--hairlen", hparams.lenght, "hair length");
  add_option(cli, "--hairstr", hparams.strength, "hair strength");
  add_option(cli, "--hairgrav", hparams.gravity, "hair gravity");
  add_option(cli, "--hairstep", hparams.steps, "hair steps");
  add_option(cli, "--output,-o", output, "output scene");
  add_option(cli, "scene", filename, "input scene", true);
  parse_cli(cli, argc, argv);

  // load scene
  auto scene_guard = std::make_unique<sio::model>();
  auto scene       = scene_guard.get();
  auto ioerror     = ""s;
  if (!load_scene(filename, scene, ioerror, cli::print_progress))
    cli::print_fatal(ioerror);

  // create procedural geometry
  if (terrain != "") {
    make_terrain(scene, get_object(scene, terrain), tparams);
  }
  if (displacement != "") {
    make_displacement(scene, get_object(scene, displacement), dparams);
  }
  if (hair != "") {
    make_hair(
        scene, get_object(scene, hairbase), get_object(scene, hair), hparams);
  }
  if (grass != "") {
    auto grasses = std::vector<sio::object*>{};
    for (auto object : scene->objects)
      if (object->name.find(grass) != scene->name.npos)
        grasses.push_back(object);
    make_grass(scene, get_object(scene, grassbase), grasses, gparams);
  }

  // make a directory if needed
  make_dir(sfs::path(output).parent_path());
  if (!scene->shapes.empty())
    make_dir(sfs::path(output).parent_path() / "shapes");
  if (!scene->subdivs.empty())
    make_dir(sfs::path(output).parent_path() / "subdivs");
  if (!scene->textures.empty())
    make_dir(sfs::path(output).parent_path() / "textures");
  if (!scene->instances.empty())
    make_dir(sfs::path(output).parent_path() / "instances");

  // save scene
  if (!save_scene(output, scene, ioerror, cli::print_progress))
    cli::print_fatal(ioerror);

  // done
  return 0;
}
