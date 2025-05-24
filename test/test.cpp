#include"http/test_http_request.h"
#include"http/test_http_context.h"
#include "http/test_http_response.h"
#include "router/test_router.h"

int main()
{
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

