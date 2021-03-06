/*------------color_quantization.cpp------------------------------------------//
*
* Purpose: We start with an octree, then we move onto an N-body simulator
*
*   Notes: Figure out why output image is all black
*          Find way to visualize particle data and octree boxes (blender?)
*          Fix pointer problem / remove p_vec from node
*
*-----------------------------------------------------------------------------*/

#include "color_quantization.h"

//using namespace cimg_library;

/*----------------------------------------------------------------------------//
* MAIN
*-----------------------------------------------------------------------------*/

int main(){
    //std::string outfile, number;
    //std::string infile = "/tmp/";
    int colores = 8;
    quantize("flower_power.png","out.bmp", 512);
/*
    #pragma omp parallel for num_threads(4)
    for (int i = 1; i <= 1200; i++){
        std::string infile = "/tmp/";
        std::string number = std::to_string(i);
        std::cout << number << '\t' << number.size() << '\n';
        for (int j = 0; j < 4 - number.size(); ++j){
            infile += "0";
        }
        infile += number + ".bmp";
        std::string outfile = "out" + number + ".bmp";

        std::cout << infile << '\n';
        if (i <= 468){
            colores = 8;
        }
        if (i > 468 && i <= 587){
            colores = 64;
        }
        if (i > 587 && i <= 697){
            colores = 256;
        }
        if (i > 697 && i <= 813){
            colores = 512;
        }
        if (i > 813 && i <= 943){
            colores = 1024;
        }
        if (i > 943 && i <= 1200){
            colores = 16384;
        }
        quantize(infile,outfile,colores);
    }
*/

}

/*----------------------------------------------------------------------------//
* SUBROUTINES
*-----------------------------------------------------------------------------*/

// Full function to quantize color
void quantize(std::string infile, std::string outfile, int colornum){
    // Defining file for output
    std::ofstream p_output("pout.dat", std::ofstream::out);
    std::ofstream output("octree0.dat", std::ofstream::out);

    // Rading in image from CImg.h
    std::string image_file = infile;
    CImg<float> image(image_file.c_str());

    // Creating the octree by reading pixels -- to come later!
    //std::vector<particle> p_vec = create_rand_dist(1, 10);
    std::vector<particle> p_vec = image_read(image, image_file);
    //p_vec = create_rand_dist(1, p_vec.size());
    std::cout << "number of pixels is: " << p_vec.size() << '\n';

    //print(p_vec[0].p);

    std::cout << "making octree" << '\n';
    node *root = make_octree(p_vec, 1);
    std::cout << "dividing octree" << '\n';
    divide_octree(root, 1);

/*
    for (auto p : root->p_vec){
        print(p->leaf->com.p);
    }
*/
    //depth_first_search(root);

    // Show all particle positions
    //particle_output(root, std::cout);
    //print(root->p_vec[0]->p);
    particle_output(root, p_output);

    //std::cout << "\n\n";
    p_output << "\n\n";

    //octree_output(root, output);
    output << root->p.x << '\t' << root->p.y << '\t' << root->p.z << '\t'
           << root->box_length << '\n';
    output << '\n' << '\n';
    octree_blender_output(root, output);

    cull_tree(root, colornum);

    quantize_color(root, p_vec, image, outfile);

    //traverse_post_order(root, [](node* n) { delete [] n; });
    delete_tree(root);
    //delete root;

}


// Function to create random distribution of particles for Octree
std::vector<particle> create_rand_dist(double box_length, int pnum){
    // Creating vector for particle positions (p_vec)
    std::vector<particle> p_vec;
    p_vec.reserve(pnum);

    // Creating random device to place particles later
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> 
        box_dist(-box_length * 0.5, box_length * 0.5);

    // Iterating over all particles to create random vecitions
    for (int i = 0; i < pnum; ++i){
        p_vec.emplace_back(vec(box_dist(gen), box_dist(gen), box_dist(gen)), 
                           PARTICLE_MASS);
    }
    
    return p_vec;
}

// Function to create particle data from image colors
std::vector<particle> image_read(CImg<float> image, std::string image_file){
    std::vector<particle> color_data((int)(image.size() / 3));

    float r, g, b;

    for (int i = 0; i < image.width(); ++i){
        for (int j = 0; j < image.height(); ++j){
            //std::cout << i << '\t' << j << '\n';
            r = image(i,j,0,0) / 255.0 - 0.5;
            g = image(i,j,0,1) / 255.0 - 0.5;
            b = image(i,j,0,2) / 255.0 - 0.5;

            color_data[j + i*image.height()] = particle(vec(r,g,b),1);

            //print(color_data[i].p);
        }
    }
    return color_data;
}

node* make_octree(std::vector<particle> &p_vec, double box_length) {
    auto root = new node(vec(), box_length, nullptr);
    root->p_vec.reserve(p_vec.size());

    for (auto& p : p_vec) {
        root->p_vec.push_back(&p);
    }
    return root;
}

// Function to create octree from vecition data
// Initially, read in root node
void divide_octree(node *curr, size_t box_threshold){

    // Divide node into 8 subnodes (boxes) to work with
    make_octchild(curr);
    
    // Iterating through all the children
    //for (auto &child : curr->children){
    for (int i = 0; i < 8; ++i){
        auto *child = &curr->children[i];
        // Determine number of particles in our current box
        for (auto p : curr->p_vec){
            if (in_box(child, p)){
                child->p_vec.push_back(p);
                child->com.p += p->p * p->mass;
                child->com.mass += p->mass;
            }
        }
        child->com.p /= child->com.mass;
        if(child->p_vec.size() != 0){
            for (auto p : child->p_vec){
                p->leaf = child;
            }
        }
        if (child->p_vec.size() > box_threshold){
            divide_octree(child, box_threshold);
        }

    }

    for (auto p : curr->p_vec){
        if (!p->leaf){
            print(p->p);
        } 
    }

/*
    if (curr->p_vec.size() <= box_threshold && 
        curr->p_vec.size() != 0){
        for (auto p : curr->p_vec){
            p->leaf = curr;
        }
    }
*/
}

// Function to check whether a vecition is within a box
bool in_box(node *curr, particle *p){
    double half_box = curr->box_length * 0.5;
    bool x_flag, y_flag, z_flag;
    if (p->p.x < 0){
        x_flag = p->p.x >= curr->p.x - half_box &&
                 p->p.x < curr->p.x + half_box;
    }
    else{
        x_flag = p->p.x > curr->p.x - half_box &&
                 p->p.x <= curr->p.x + half_box;
    }

    if (p->p.y < 0){
        y_flag = p->p.y >= curr->p.y - half_box &&
                 p->p.y < curr->p.y + half_box;
    }
    else{
        y_flag = p->p.y > curr->p.y - half_box &&
                 p->p.y <= curr->p.y + half_box;
    }

    if (p->p.z < 0){
        z_flag = p->p.z >= curr->p.z - half_box &&
                 p->p.z < curr->p.z + half_box;
    }
    else{
        z_flag = p->p.z > curr->p.z - half_box &&
                 p->p.z <= curr->p.z + half_box;
    }

    return x_flag && y_flag && z_flag;

}

// Function to check whether a vecition is within a box
bool in_box(node *curr, particle &p){
    double half_box = curr->box_length * 0.5;
    bool x_flag, y_flag, z_flag;

    if (p.p.x < 0){
        x_flag = p.p.x >= curr->p.x - half_box &&
                 p.p.x < curr->p.x + half_box;
    }
    else{
        x_flag = p.p.x > curr->p.x - half_box &&
                 p.p.x <= curr->p.x + half_box;
    }

    if (p.p.y < 0){
        y_flag = p.p.y >= curr->p.y - half_box &&
                 p.p.y < curr->p.y + half_box;
    }
    else{
        y_flag = p.p.y > curr->p.y - half_box &&
                 p.p.y <= curr->p.y + half_box;
    }

    if (p.p.z < 0){
        z_flag = p.p.z >= curr->p.z - half_box &&
                 p.p.z < curr->p.z + half_box;
    }
    else{
        z_flag = p.p.z > curr->p.z - half_box &&
                 p.p.z <= curr->p.z + half_box;
    }

    return x_flag && y_flag && z_flag;

}


// Function to create 8 children node for octree
void make_octchild(node *curr){

    curr->children = new node[8];

    double node_length = curr->box_length * 0.5;
    double quarter_box = curr->box_length * 0.25;

    // iterating through vecsible locations for new children nodes
    // This was written by laurensbl
    for (int k = -1; k <= 1; k+= 2){
        for (int j = -1; j <= 1; j += 2){
            for (int i = -1; i <= 1; i +=2){
                int n = 2 * k + j + (i+1)/2 + 3;
                //curr->children[n] = new node();
                curr->children[n].parent = curr;
                curr->children[n].box_length = node_length;
                curr->children[n].p.z = curr->p.z + k * quarter_box;
                curr->children[n].p.y = curr->p.y + j * quarter_box;
                curr->children[n].p.x = curr->p.x + i * quarter_box;
            }
        }
    }
}

// Function to perform a depth first search of octree
void depth_first_search(node *curr){
    if (!curr){
        return;
    }
    print(curr->com.p);

    if (curr->children){
        for (int i = 0; i < 8; ++i){
            depth_first_search(&curr->children[i]);
        }
    }
}


// Function to output vertex positions of cube(s)
void octree_output(node *curr, std::ostream &output){
    if (!curr){
        return;
    }

    // Outputting current node vertices

    // find the lower left vertex
    vec llv = vec(curr->p.x - curr->box_length * 0.5,
                  curr->p.y - curr->box_length * 0.5,
                  curr->p.z - curr->box_length * 0.5);

    output << llv.x << '\t' << llv.y << '\t' << llv.z << '\n';
    output << llv.x << '\t' << llv.y << '\t' 
           << llv.z + curr->box_length << '\n';
    output << llv.x << '\t' << llv.y + curr->box_length << '\t' 
           << llv.z + curr->box_length << '\n';
    output << llv.x << '\t' << llv.y + curr->box_length << '\t' 
           << llv.z << '\n';
    output << llv.x << '\t' << llv.y << '\t' << llv.z << '\n';
    output << '\n' << '\n';

    output << llv.x + curr->box_length << '\t' << llv.y << '\t' 
           << llv.z << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y << '\t' 
           << llv.z + curr->box_length << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y + curr->box_length 
           << '\t' << llv.z +curr->box_length << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y + curr->box_length 
           << '\t' << llv.z << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y << '\t' 
           << llv.z << '\n';
    output << '\n' << '\n';

    output << llv.x << '\t' << llv.y << '\t' << llv.z << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y << '\t' 
           << llv.z << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y + curr->box_length 
           << '\t' << llv.z << '\n';
    output << llv.x << '\t' << llv.y + curr->box_length << '\t' 
           << llv.z << '\n';
    output << llv.x << '\t' << llv.y << '\t' << llv.z << '\n';
    output << '\n' << '\n';

    output << llv.x << '\t' << llv.y << '\t' 
           << llv.z + curr->box_length << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y << '\t' 
           << llv.z + curr->box_length << '\n';
    output << llv.x + curr->box_length << '\t' << llv.y + curr->box_length 
           << '\t' << llv.z + curr->box_length << '\n';
    output << llv.x << '\t' << llv.y + curr->box_length << '\t' 
           << llv.z + curr->box_length << '\n';
    output << llv.x << '\t' << llv.y << '\t' 
           << llv.z + curr->box_length << '\n';
    output << '\n' << '\n';

    // Recursively outputting internal boxes
    if (curr->children){
        for (int i = 0; i < 8; ++i){
            octree_output(&curr->children[i], output);
        }
    }
 
}

void octree_blender_output(node *curr, std::ostream &output){
    if (!curr){
        return;
    }

    if (curr->children){
        for (int i = 0; i < 8; ++i){
            output << curr->children[i].p.x << '\t' << curr->children[i].p.y
                   << '\t' << curr->children[i].p.z << '\t'
                   << '\t' << curr->children[i].box_length << '\n';
        }

        output << '\n' << '\n';
    }

    // Recursively outputting internal boxes
    if (curr->children){
        for (int i = 0; i < 8; ++i){
            octree_blender_output(&curr->children[i], output);
        }
    }

}

// Function to output particle positions
void particle_output(node *curr, std::ostream &p_output){
    if (!curr){
        return;
    }

    for (size_t i = 0; i < curr->p_vec.size(); ++i){
        p_output << curr->p_vec[i]->p.x << '\t' << curr->p_vec[i]->p.y << '\t' 
                 << curr->p_vec[i]->p.z << '\n';

    }

}

// Function to merge unnecessary nodes
void cull_tree(node *root, int cull_num){

    int depth = 0;
    std::vector <node*> tree_vec;
    create_tree_vec(root, tree_vec, depth);
    std::sort(tree_vec.begin(), tree_vec.end(), node_ineq);


    for (int i = 0; i < cull_num; i++){
        //std::cout << i << '\t' << tree_vec[i]->p_vec.size() << '\n';
        for (auto &p : tree_vec[i]->p_vec){
            p->leaf = tree_vec[i];
        }
    }

}

// Function to make vector of nodes
void create_tree_vec(node *curr, std::vector<node*> &tree_vec, int depth){
    if (!curr){
        return;
    }
    depth++;

    curr->depth = depth;
    tree_vec.push_back(curr);

    if (curr->children){
        for (int i = 0; i < 8; ++i){
            auto *child = &curr->children[i];
            create_tree_vec(child, tree_vec, depth);
        }
    }

}


// Function to quantize the color
void quantize_color(node *root, std::vector<particle> color_data, 
                    CImg<float> &image, std::string outfile){

    vec color_p;
    float r, g, b;

    // Iterating through the image
    for (int i = 0; i < image.width(); ++i){
        for (int j = 0; j < image.height(); ++j){
            if (j + i * image.height() % 100 == 0){
                std::cout << j + i * image.height() << '\n';
            }
            color_p = root->p_vec[j + i * image.height()]->leaf->com.p;
            r = (color_p.x + 0.5) * 255;
            g = (color_p.y + 0.5) * 255;
            b = (color_p.z + 0.5) * 255;
            image(i, j, 0, 0) = r;
            image(i, j, 0, 1) = g;
            image(i, j, 0, 2) = b;
            //std::cout << r << '\t' << g << '\t' << b << '\n';

            //image(i, j, 0, 0) = (color_p.p.x + 0.5) * 255;
            //image(i, j, 0, 1) = (color_p.p.y + 0.5) * 255;
            //image(i, j, 0, 2) = (color_p.p.z + 0.5) * 255;

        }
    }

    image.save(outfile.c_str());
}

// Function to delete the tree
void delete_tree(node* curr){
    if (!curr) {
        return;
    }

    if (curr->children){
        for (int i = 0; i < 8; ++i){
            delete_tree(&curr->children[i]);
        }
        delete [] curr->children;
    }

    if (!curr->parent){
        delete curr;
    }

}
