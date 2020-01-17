#pragma once
class Context;

bool route_client_set_default(Context* context);
bool route_client_unset_default(Context* context);

bool route_server_add_client(Context* context);
bool route_server_remove_client(Context* context);