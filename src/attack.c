/******************
 * Created by Apollos on 2020/1/10.
 *****************/
#include "../include/attack.h"
#include <assert.h>
#include <stdlib.h>
#include <memory.h>


Attack* create_attackMsg(int eventId, int attackerId, int beAttackedPlayerId, int attack_x, int attack_y,int faceToRight){
    Attack *attack=(Attack*)calloc(1, sizeof(Attack));
    assert(attack!=NULL);
    attack->eventId=eventId;
    attack->attackerId=attackerId;
    attack->beAttackedPlayerId = beAttackedPlayerId;
    attack->attack_x = attack_x;
    attack->attack_y = attack_y;
    attack->faceToRight = faceToRight;
    return attack;
}

int isInAttackRange(Attack *a){
    int diff_x=a->attack_x-a->beAttacked_x;
    int diff_y=a->attack_y=a->beAttacked_y;
    if ((-5<=diff_y&&diff_y<=5)&&((0<=diff_x&&diff_x<=5&&!a->faceToRight)||(-5<=diff_x&&diff_x<0&&a->faceToRight))) {
        return 1;
    }
    return 0;
}



static int indexof(VectorAttack *va,Attack *attack){
    int i=0;
    for (;i<va->counter;i++){
        if(va->attacks[i].eventId==attack->eventId){
            return i;
        }
        return -1;
    }
}

static void encapacity(VectorAttack *va){
    if(va->counter>=va->max_counter){
        Attack *new_attacks=(Attack*)calloc(va->counter+5, sizeof(Attack));
        assert(new_attacks!=NULL);
        memcpy(new_attacks,va->attacks, sizeof(Attack)*va->counter);
        free(va->attacks);
        va->attacks=new_attacks;
        va->max_counter+=5;
    }
}

VectorAttack* create_vector_attack(void){
    VectorAttack *va=(VectorAttack*)calloc(1, sizeof(VectorAttack));
    assert(va!=NULL);
    va->attacks=(Attack*)calloc(50, sizeof(Attack));
    assert(va->attacks!=NULL);
    va->counter=0;
    va->max_counter=0;
    return va;
}

void destroy_vector_attack(VectorAttack *va){
    assert(va!=NULL);
    free(va->attacks);
    free(va);
}

Attack * get_attack(VectorAttack *va,int index){
    assert(va!=NULL);
    if(index<0||index>va->counter-1)
        return 0;
    return &(va->attacks[index]);
}

Attack * find_attack_by_id(VectorAttack *va,int id){
    int i=0;
    for (;i<va->counter;i++){
        Attack *attack=get_attack(va,i);
        if(attack->eventId==id){
            return attack;
        }
        return NULL;
    }
}

void remove_attack(VectorAttack *va,Attack *attack){
    assert(va!=NULL);
    int index=indexof(va,attack);
    if(index==-1)return;
    int i=index;
    for(;i<va->counter-1;i++){
        va->attacks[i]=va->attacks[i+1];
    }
    va->counter--;
}

void add_attack(VectorAttack *va,Attack attack){
    assert(va!=NULL);
    encapacity(va);
    va->attacks[va->counter++]=attack;
}


