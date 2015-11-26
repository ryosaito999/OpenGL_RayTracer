/**
 * ray_tracer.cpp
 * CS230
 * -------------------------------
 * Implement ray tracer here.
 */

#define SET_RED(P, C)   (P = (((P) & 0x00ffffff) | ((C) << 24)))
#define SET_GREEN(P, C)  (P = (((P) & 0xff00ffff) | ((C) << 16)))
#define SET_BLUE(P, C) (P = (((P) & 0xffff00ff) | ((C) << 8)))

#include "ray_tracer.h"
using namespace std;

const double Object::small_t=1e-6;
//--------------------------------------------------------------------------------
// utility functions
//--------------------------------------------------------------------------------
double sqr(const double x)
{
    return x*x;
}

Pixel Pixel_Color(const Vector_3D<double>& color)
{
    //pixel shader is implemented :D
    Pixel pixel=0;
    SET_RED(pixel,(unsigned char)(min(color.x,1.0)*255));
    SET_GREEN(pixel,(unsigned char)(min(color.y,1.0)*255));
    SET_BLUE(pixel,(unsigned char)(min(color.z,1.0)*255));
    return pixel;
}
//--------------------------------------------------------------------------------
// Shader
//--------------------------------------------------------------------------------
Vector_3D<double> Phong_Shader::
Shade_Surface(const Ray& ray,const Object& intersection_object,const Vector_3D<double>& intersection_point,const Vector_3D<double>& same_side_normal) const
{
    Vector_3D<double> color;
    for (unsigned int i = 0; i< world.lights.size() ; ++i){

        Vector_3D<double> light =  world.lights[i]->position - intersection_point;
        light.Normalize();

        double diffuseLight = Vector_3D<double>:: Dot_Product(light, same_side_normal);
        diffuseLight = max(0.0, diffuseLight);

        Vector_3D<double> v = ray.endpoint - intersection_point;
        v.Normalize();

        Vector_3D<double> r = (same_side_normal  * diffuseLight * 2) - light;
        r.Normalize();

        double spec = max(0.0, Vector_3D<double>:: Dot_Product(v,r));
        Vector_3D<double> col = world.lights[i]->Emitted_Light(ray);

        //color += color_ambient*col*1.0;        
        Ray shadowRay(intersection_point,world.lights[i]->position );

        if( world.enable_shadows){
            for ( int j = 0; j < world.objects.size(); ++j){

                if(!world.objects[j] -> Intersection(shadowRay) )                   
                    color +=  color_diffuse * col * diffuseLight + color_specular *col * pow(spec,specular_power);//anything else?;
            }
         }
         else{
            color += color_diffuse * col * diffuseLight + color_specular *col * pow(spec,specular_power);//anything else?;
        }
    }
    return color;
}

Vector_3D<double> Reflective_Shader::
Shade_Surface(const Ray& ray,const Object& intersection_object,const Vector_3D<double>& intersection_point,const Vector_3D<double>& same_side_normal) const
{
    Vector_3D<double> color;

    // TODO: determine the color

    return color;
}

Vector_3D<double> Flat_Shader::
Shade_Surface(const Ray& ray,const Object& intersection_object,const Vector_3D<double>& intersection_point,const Vector_3D<double>& same_side_normal) const
{
    return color;
}

//--------------------------------------------------------------------------------
// Objects
//--------------------------------------------------------------------------------
// determine if the ray intersects with the sphere
// if there is an intersection, set t_max, current_object, and semi_infinite as appropriate and return true
bool Sphere::
Intersection(Ray& ray) const
{
    // TODO
    
    double a = Vector_3D<double>::Dot_Product(ray.direction, ray.direction);
    double b = 2 * Vector_3D<double>::Dot_Product(ray.direction, ray.endpoint - center);
    double c = Vector_3D<double>::Dot_Product(ray.endpoint - center, ray.endpoint - center) - radius * radius;
    
    double g = b * b - 4 * a * c;
    
    if( g < 0 ){
        return false;
    }


    else{
        double t = 0;
        double t1 =(-b + sqrt( g) ) / (2.0 * a);
        double t2 = (-b - sqrt( g) ) / (2.0 * a);
        

        if( t1 >= 0 && t2 < 0 ) 
            t = t1;

        else if( t2 >= 0 && t1 < 0 ) 
            t = t2;

        else if( t1 >= 0 && t2 >= 0 ) 
            t = min(t1, t2);

        else
            return false;
    
        if( ray.semi_infinite == true ){
            ray.current_object = this;
            ray.semi_infinite = false;
            ray.t_max = t;
            return true;
        }
        else if(ray.semi_infinite == false &&  t < ray.t_max){
            ray.current_object = this;
            ray.t_max = t;
            return true;
        }
        
        return false;
    }

}

Vector_3D<double> Sphere::
Normal(const Vector_3D<double>& location) const
{
    Vector_3D<double> normal = ( location - center);
    normal.Normalize();
    // TODO: set the normal
    return normal;
}

// determine if the ray intersects with the sphere
// if there is an intersection, set t_max, current_object, and semi_infinite as appropriate and return true
bool Plane::
Intersection(Ray& ray) const
{
    double n = Vector_3D <double> :: Dot_Product( normal , ( x1 - ray.endpoint));
    double d = Vector_3D <double> :: Dot_Product( normal ,  ray.direction );
    double t = n/d;    

    if( ray.semi_infinite == true ){
        ray.current_object = this;
        ray.semi_infinite = false;
        ray.t_max = t;
        return true;
    }
    else if(ray.semi_infinite == false  && t < ray.t_max){
        ray.current_object = this;
        ray.t_max = t;
        return true;
    }
    
    return false;
    
}
Vector_3D<double> Plane::
Normal(const Vector_3D<double>& location) const
{
    return normal;
}
//--------------------------------------------------------------------------------
// Camera
//--------------------------------------------------------------------------------
// Find the world position of the input pixel
Vector_3D<double> Camera::
World_Position(const Vector_2D<int>& pixel_index)
{
    Vector_2D<double> d = film.pixel_grid.X(pixel_index);
    Vector_3D<double> result = + focal_point + horizontal_vector * d.x + vertical_vector * d.y; 
    return result;
}
//--------------------------------------------------------------------------------
// Render_World
//--------------------------------------------------------------------------------
// Find the closest object of intersection and return a pointer to it
//   if the ray intersects with an object, then ray.t_max, ray.current_object, and ray.semi_infinite will be set appropriately
//   if there is no intersection do not modify the ray and return 0
const Object* Render_World::
Closest_Intersection(Ray& ray)
{
    for( int i =0; i< objects.size(); ++i){
        objects[i]->Intersection(ray);
    }
    
    return ray.current_object;

}

// set up the initial view ray and call 
void Render_World::
Render_Pixel(const Vector_2D<int>& pixel_index)
{
    // TODO
    Ray dummy_root;
    Ray ray( camera.position, camera.World_Position(pixel_index) - camera.position);
    ray.t_max = 1000;
    ray.semi_infinite = true;
    Vector_3D<double> color=Cast_Ray(ray,dummy_root);
    camera.film.Set_Pixel(pixel_index,Pixel_Color(color));
}

// cast ray and return the color of the closest intersected surface point, 
// or the background color if there is no object intersection
Vector_3D<double> Render_World::
Cast_Ray(Ray& ray,const Ray& parent_ray)
{
    // TODO
    Vector_3D<double> color;
    const Object * obj = Closest_Intersection(ray);

    if( obj ){ 
        Vector_3D<double> intersect = ray.Point(ray.t_max);
        Vector_3D<double> normal = obj->Normal(intersect);
        return obj->material_shader->Shade_Surface(ray, *obj, intersect, normal);
    }
    return color;
}
