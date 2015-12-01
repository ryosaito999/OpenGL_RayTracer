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
    for (int i = 0; i< world.lights.size() ; ++i){
        //color += color_ambient*col*1.0; guess you dont need tjis?       

        //diffuse stuff
        Vector_3D<double> light =  world.lights[i]->position - intersection_point;
        light.Normalize();

        double diffuseLight = Vector_3D<double>:: Dot_Product(light, same_side_normal);
        diffuseLight = max(0.0, diffuseLight);

        //specular stuff
        Vector_3D<double> v = ray.endpoint - intersection_point;
        Vector_3D<double> r = (same_side_normal  * diffuseLight * 2) - light;
        r.Normalize();
        v.Normalize();

        double spec = max(0.0, Vector_3D<double>:: Dot_Product(v,r));
        
        Vector_3D<double> col = world.lights[i]->Emitted_Light(ray);
        Ray shadowRay(intersection_point + same_side_normal* intersection_object.small_t, light );

        if( world.enable_shadows){
                const Object *obj = world.Closest_Intersection(shadowRay);

                if(!obj)
                    color += col * color_ambient*.2 + color_diffuse * col * diffuseLight + color_specular *col * pow(spec,specular_power);//anything else?;
         }

         else{
            color+= col * color_ambient*.2  + color_diffuse * col * diffuseLight + color_specular *col * pow(spec,specular_power);//anything else?;
        }
    }

///    reflector.direction = -(ray.direction - same_side_normal * 2* Vector_3D<double>::Dot_Product(ray.direction , same_side_normal))  ;
   	

    return color;
}

Vector_3D<double> Reflective_Shader::
Shade_Surface(const Ray& ray,const Object& intersection_object,const Vector_3D<double>& intersection_point,const Vector_3D<double>& same_side_normal) const
{   

    //R = V – 2 * (V·N) * N 
    //http://paulbourke.net/geometry/reflected/

    //add to current phong shader
    Vector_3D<double> color = Phong_Shader::Shade_Surface(ray, intersection_object, intersection_point, same_side_normal);

    //create new ray to cast -> look at slides pls
    Ray reflector;
    
    //reflection needs to go back to camera so start with intersect -> camera
    reflector. endpoint = intersection_point + same_side_normal * intersection_object.small_t;
    reflector.direction = ray.direction - same_side_normal * 2* Vector_3D<double>::Dot_Product(ray.direction , same_side_normal)  ;
    
    //cast the ray and add it to the phong shader
    color +=  world.Cast_Ray(reflector, ray) * reflectivity  ;

    return color;
}

Vector_3D<double> Flat_Shader::
Shade_Surface(const Ray& ray,const Object& intersection_object,const Vector_3D<double>& intersection_point,const Vector_3D<double>& same_side_normal) const
{
    return color;
}

Vector_3D<double> CheckerBoard_Shader::
Shade_Surface(const Ray& ray,const Object& intersection_object,const Vector_3D<double>& intersection_point,const Vector_3D<double>& same_side_normal) const
{


    Vector_3D <double>color;


    for (int i = 0; i< world.lights.size() ; ++i){
        //color += color_ambient*col*1.0; guess you dont need tjis?       

        //diffuse stuff
        Vector_3D<double> light =  world.lights[i]->position - intersection_point;
        light.Normalize();

        double diffuseLight = Vector_3D<double>:: Dot_Product(light, same_side_normal);
        diffuseLight = max(0.0, diffuseLight);

        //specular stuff
        Vector_3D<double> v = ray.endpoint - intersection_point;
        Vector_3D<double> r = (same_side_normal  * diffuseLight * 2) - light;
        r.Normalize();
        v.Normalize();

        double spec = max(0.0, Vector_3D<double>:: Dot_Product(v,r));
        
        Vector_3D<double> col = world.lights[i]->Emitted_Light(ray);
        Ray shadowRay(intersection_point + same_side_normal* intersection_object.small_t, light );

        if( world.enable_shadows){
                const Object *obj = world.Closest_Intersection(shadowRay);

                if(!obj){
                    double x = intersection_point.x;
                    double z = intersection_point.z;

                    //shift to the right by 1
                    if(z < 0)
                        z = -z+1;
                    if(x < 0)
                        x = -x+1;

                    int n = ((int)x + (int)z)%2;

                    if( n == 1)
                        color= Vector_3D<double>(1,1,1);
                }
         }

         else{
            double x = intersection_point.x;
            double z = intersection_point.z;

            //shift to the right by 1
            if(z < 0)
                z = -z+1;
            if(x < 0)
                x = -x+1;

            int n = ((int)x + (int)z)%2;

            if( n == 1)
                color= Vector_3D<double>(1,1,1);
                
        }
    }


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
    
    double a = Vector_3D<double>::Dot_Product(ray.direction, ray.direction);
    double b = 2 * Vector_3D<double>::Dot_Product(ray.direction, ray.endpoint - center);
    double c = Vector_3D<double>::Dot_Product(ray.endpoint - center, ray.endpoint - center) - radius * radius;
    double g = b * b - 4 * a * c;
    
    if( g < 0 )
        return false;
    
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
    
        // if ray hasnt hit anything yet, then set semi to false and mark 
        //new object as rays intersection. T is intersection point
        if( ray.semi_infinite == true && t>small_t ){
            ray.current_object = this;
            ray.t_max = t;
            ray.semi_infinite = false;

            return true;
        }
        // if ray is already set to an object, check if current obj is closer
        // if it is set t to new value
        else if(ray.semi_infinite == false &&  t < ray.t_max  && t>small_t){
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

    if( t >= 0){
	    if( ray.semi_infinite == true && t>small_t ){
	        ray.current_object = this;
	        ray.t_max = t;
	        ray.semi_infinite = false;

	        return true;
	    }

	    else if(ray.semi_infinite == false  && t < ray.t_max  && t>small_t){
	        ray.current_object = this;
	        ray.t_max = t;
	        return true;
	    }
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
//~ float RandomFloat(float min, float max)
//~ {
    //~ // this  function assumes max > min, you may want 
    //~ // more robust error checking for a non-debug build
    //~ //assert(max > min); 
    //~ float random = ((float) rand()) / (float) RAND_MAX;
//~ 
    //~ // generate (in your case) a float between 0 and (4.5-.78)
    //~ // then add .78, giving you a float between .78 and 4.5
    //~ float range = max - min;  
    //~ return (random*range) + min;
//~ }

Vector_3D<double> Camera::
World_Position(const Vector_2D<int>& pixel_index)
{
    Vector_2D<double> d = film.pixel_grid.X(pixel_index);
    Vector_3D<double> result =  focal_point + horizontal_vector *( d.x )  + vertical_vector * (d.y ) ; 
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
    Ray ray;
    Vector_3D<double> color;
    Vector_3D <double>pixelPos = camera.World_Position(pixel_index);
    float AAfactor = 4; 
    for ( float i = 0; i < AAfactor; i++){
        for ( float j = 0; j < AAfactor ; j++){
            ray.endpoint = camera.position;
            ray.direction =pixelPos - camera.position;

            ray.t_max = 1000;
            ray.semi_infinite = true;
            color+= Cast_Ray(ray,dummy_root) * (1/(AAfactor*2)) ;
            
            //ummm i got this by gettting the pxiel width manually 
            // i dont know how to get the actual width
            pixelPos.x += camera.film.pixel_grid.dx/AAfactor;

        }
        
        pixelPos.y += camera.film.pixel_grid.dy/AAfactor;

    }

    camera.film.Set_Pixel(pixel_index,Pixel_Color(color));
}

// cast ray and return the color of the closest intersected surface point, 
// or the background color if there is no object intersection
Vector_3D<double> Render_World::
Cast_Ray(Ray& ray,const Ray& parent_ray)
{
    float coef = .25;
    // TODO
    Vector_3D<double> color;
    const Object *obj = Closest_Intersection(ray);
    
    if( obj ){ 
        Vector_3D<double> intersect = ray.Point(ray.t_max);
        Vector_3D<double> normal = obj->Normal(intersect);
        color =  obj->material_shader->Shade_Surface(ray, *obj, intersect, normal)  ;
    }
        
    

    return color;
}
