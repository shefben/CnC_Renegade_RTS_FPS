#ifndef TT_INCLUDE__SCRIPTCOMMANDS_H
#define TT_INCLUDE__SCRIPTCOMMANDS_H

void Set_Animation ( GameObject * obj, const char * anim_name, bool looping, const char * sub_obj_name, float start_frame, float end_frame, bool is_blended);
void Apply_Damage(GameObject * object, float amount, const char * warhead_name, GameObject * damager);
#endif