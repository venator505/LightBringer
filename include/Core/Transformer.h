#ifndef YYLB_TRANSFORMER_H
#define YYLB_TRANSFORMER_H
#include <math/Matrix.h>
#include <Core/Camera.h>
namespace YYLB
{
    //负责坐标转换,并输出相应
    struct Transformer
    {
        Matrix4f m_m2w; //模型变化矩阵
        Matrix4f m_w2v; //视图变换矩阵
        Matrix4f m_v2p; //透视投影变换矩阵
        Matrix4f m_p2s; //视口变换矩阵

        //物体移动时重新计算
        void set_model_to_world(Vec3f &world_pos);

        //相机位置改变时重新计算
        void set_world_to_view(Camera *cam);

        //相机变化时重新计算
        void set_view_to_project(Camera *cam);

        //屏幕尺寸改变时重新计算
        void set_projection_to_screen(int &w, int &h);

        //转换输出
        Vec4f vertex_output(Vec3f &local_pos, Vec3f &world_pos);
    };

}
#endif