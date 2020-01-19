#pragma once

class Context;
int tun_create(char if_name[16], const char *wanted_name);
bool tun_setup(Context* context);
bool tun_remove(Context* context);