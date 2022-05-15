#include <iostream>
#include "Transformer.h"
namespace ylb
{
    glm::mat4 Transformer::calc_matrix_world(glm::vec3 &world_pos)
    {
        auto world = glm::mat4 (1);
        world[0][3] = world_pos.x;
        world[1][3] = world_pos.y;
        world[2][3] = world_pos.z;
        return world;
    }

    void Transformer::set_world_to_view(Camera *cam)
    {
        view = glm::mat4 (1);
        auto g = cam->look_at;
        g = glm::normalize(g);
        auto& t = cam->up;

        auto w = g * -1.0f;         w = glm::normalize(w);
        auto u = glm::cross(t,w);   u = glm::normalize(u);
        auto v = glm::cross(w,u);

        view[0][0] = u.x;view[0][1] = u.y;view[0][2] = u.z;
        view[1][0] = v.x;view[1][1] = v.y;view[1][2] = v.z;
        view[2][0] = w.x;view[2][1] = w.y;view[2][2] = w.z;
        view[0][3] = cam->position_world.x * -1;
        view[1][3] = cam->position_world.y * -1;
        view[2][3] = cam->position_world.z * -1;
    }

    void Transformer::set_view_to_project(Camera *cam,PROJECTION_MODE mode)
    {
        cam->mode = mode;
        if(mode == PROJECTION_MODE::PERSPECTIVE)
            cam->set_perspective_matrix(projection);
        if(mode == PROJECTION_MODE::ORTHOGONAL)
            cam->set_orthogonal_matrix(projection);
    }

    void Transformer::set_projection_to_screen(int &w, int &h)
    {
        view_port = glm::mat4 (1);
        view_port[0][0] = w * 1.f / 2;view_port[0][3] = (w-1) / 2;
        view_port[1][1] = h * 1.f / 2;view_port[1][3] = (h-1) / 2;
    }

}