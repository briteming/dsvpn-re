#include "state/Context.h"

int main() {
   auto context = Context::GetInstance();
   auto res = context->Init();
}