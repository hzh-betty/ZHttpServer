#include"http/test_http_request.h"
#include"http/test_http_context.h"
#include "http/test_http_response.h"

#include "router/test_router.h"

#include "session/test_session.h"
#include "session/test_memory_storage.h"
#include "session/test_session_manager.h"

#include "middleware/test_middleware_chain.h"
#include "middleware/test_cors_middle.h"

#include "db_pool/test_db_connection.h"
#include "db_pool/test_db_pool.h"

#include "ssl/test_ssl_config.h"
#include "ssl/test_ssl_context.h"
#include "ssl/test_ssl_connection.h"
int main()
{
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

