//
// Created by ziyinqu on 10/2/17.
//
#pragma once
#ifndef MPM_TRANSFER_H
#define MPM_TRANSFER_H

#include "global.h"
#include "Eigen/Eigen"
#include "interpolation.h"

#define USEAPIC true

using namespace std;

void transferG2P(vector<Particle>& particles, vector<GridAttr>& gridAttrs, const GridInfo gridInfo, float dt, float alpha){

    int iterationNum = particles.size();
    Matrix3f wp = Matrix3f::Zero();
    Matrix3f dwp = Matrix3f::Zero();
    Vector3i baseNode = Vector3i::Zero();
    int H = gridInfo.H;
    int L = gridInfo.L;

    for(int iter =0; iter < iterationNum; iter++) {
        Vector3f vpic = Vector3f::Zero();
        Vector3f vflip = Vector3f::Zero();
        Vector3f index_Space = particles[iter].posP/gridInfo.dx;
        QuadraticInterpolation(index_Space, baseNode, wp, dwp);
        for (int i = 0; i < 3; i++) {
            float wi = wp(0, i);
            int node_i = baseNode(0) + i;
            for (int j = 0; j < 3; j++) {
                float wij = wi * wp(1, j);
                int node_j = baseNode(1) + j;
                for (int k = 0; k < 3; k++) {
                    float wijk = wij * wp(2, k);
                    int node_k = baseNode(2) + k;
                    int index = node_i * H * L + node_j * L + node_k;

                    Vector3f gridIndex = Vector3f(node_i,node_j,node_k);
                    particles[iter].BP += wijk * gridAttrs[index].velG * (gridIndex.transpose() - index_Space.transpose());

                    // calculate PIC and FLIP part velocity
                    vpic += wijk * gridAttrs[index].velG;
                    vflip += wijk * (gridAttrs[index].velG - gridAttrs[index].velGn);
                }
            }
        }
        // update particles velocity and position
        if(!USEAPIC) {
            particles[iter].velP = (1 - alpha) * vpic + alpha * vflip;
        }
        else {
            particles[iter].velP = vpic;
        }

        particles[iter].posP += dt * particles[iter].velP;
    }
}

void transferP2G(vector<Particle>& particles, vector<GridAttr>& gridAttrs, const GridInfo gridInfo, std::vector<int>& active_nodes)
{
    int iterationNum = particles.size();
    Matrix3f wp = Matrix3f::Zero();
    Matrix3f dwp = Matrix3f::Zero();
    Vector3i baseNode = Vector3i::Zero();
    int H = gridInfo.H;
    int L = gridInfo.L;
    for(int iter =0; iter < iterationNum; iter++)
    {
        Vector3f index_Space = particles[iter].posP/gridInfo.dx;
        QuadraticInterpolation(index_Space, baseNode, wp, dwp);
        for (int i = 0; i < 3; i++){
            float wi = wp(0,i);
            int node_i = baseNode(0) + i;
            for (int j = 0; j < 3; j++){
                float wij = wi * wp(1,j);
                int node_j = baseNode(1) + j;
                for (int k = 0; k < 3; k++){
                    float wijk = wij * wp(2, k);
                    int node_k = baseNode(2) + k;
                    int index = node_i * H * L + node_j * L + node_k;
                    // grid mass transfer
                    gridAttrs[index].massG += wijk * particles[iter].massP;

                    // calculate APIC things
                    Vector3f gridNode = Vector3f(node_i,node_j,node_k);
                    Vector3f plus = 4*particles[i].BP*(gridNode-index_Space);

                    // grid velocity transfer
                    if(USEAPIC) {
                        gridAttrs[index].velGn += wijk * particles[iter].massP * (particles[iter].velP + plus);
                    } else{
                        gridAttrs[index].velGn += wijk * particles[iter].massP * particles[iter].velP;
                    }

                }
            }
        }
    }

    for (int iter = 0; iter < gridAttrs.size(); iter++){
        if (gridAttrs[iter].massG != 0){
            active_nodes.push_back(iter);
            gridAttrs[iter].velGn = gridAttrs[iter].velGn / gridAttrs[iter].massG ;
        }
        else{
            gridAttrs[iter].velGn = Vector3f::Zero();
        }
    }

}
#endif //MPM_TRANSFER_H
