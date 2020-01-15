#include "state/Context.h"

int main() {
   auto context = Context::GetInstance();
   context->Init();
}