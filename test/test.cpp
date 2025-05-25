#include"http/test_http_request.h"
#include"http/test_http_context.h"
#include "http/test_http_response.h"

#include "router/test_router.h"

#include "session/test_session.h"
#include "session/test_session_storage.h"
#include "session/test_session_manager.h"

int main()
{
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

