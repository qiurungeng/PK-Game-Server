/******************
 * Created by Apollos on 2020/1/10.
 *****************/

#ifndef GAMESERVER_ATTACK_H
#define GAMESERVER_ATTACK_H
typedef struct {
    int eventId;
    int attackerId;
    int beAttackedPlayerId;
    int attack_x;
    int attack_y;
    int faceToRight;
    int beAttacked_x;
    int beAttacked_y;
}Attack;

typedef struct {
    Attack *attacks;        //攻击事件动态数组
    int counter;            //动态数组里攻击事件的总数
    int max_counter;
}VectorAttack;

extern Attack* create_attackMsg(int eventId, int attackerId, int beAttackedPlayerId, int attack_x, int attack_y,int faceToRight);
extern int isInAttackRange();

extern VectorAttack* create_vector_attack(void);
extern void destroy_vector_attack(VectorAttack *);
extern Attack * get_attack(VectorAttack *,int index);
extern Attack * find_attack_by_id(VectorAttack *,int id);
extern void remove_attack(VectorAttack *,Attack *);
extern void add_attack(VectorAttack *,Attack );
#endif //GAMESERVER_ATTACK_H
