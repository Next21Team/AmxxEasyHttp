#include <gtest/gtest.h>

#include <EasyHttpModule.h>

TEST(EasyHttpModuleTest, SendRequestRejectsUnknownQueue)
{
    EasyHttpModule module("test-ca.pem");

    OptionsData options;
    options.queue_id = static_cast<QueueId>(999);

    const RequestId request_id = module.SendRequest(
        ezhttp::RequestMethod::HttpGet,
        "https://example.com",
        options
    );

    EXPECT_EQ(RequestId::Null, request_id);
}

TEST(EasyHttpModuleTest, SendRequestRejectsInvalidPluginEndBehaviour)
{
    EasyHttpModule module("test-ca.pem");

    OptionsData options;
    options.plugin_end_behaviour = static_cast<PluginEndBehaviour>(999);

    const RequestId request_id = module.SendRequest(
        ezhttp::RequestMethod::HttpGet,
        "https://example.com",
        options
    );

    EXPECT_EQ(RequestId::Null, request_id);
}
