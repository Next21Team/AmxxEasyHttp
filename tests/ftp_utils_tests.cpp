#include <filesystem>

#include <gtest/gtest.h>

#include <utils/ftp_utils.h>

TEST(FtpUtilsTest, ConstructFtpUrlEncodesCredentials)
{
    EXPECT_EQ(
        "ftp://user%40mail.com:p%40ss%3Aword@example.com/files/demo.dem",
        utils::ConstructFtpUrl("user@mail.com", "p@ss:word", "example.com", "/files/demo.dem")
    );
}

TEST(FtpUtilsTest, ConstructFtpUrlOmitsEmptyCredentials)
{
    EXPECT_EQ(
        "ftp://example.com/files/demo.dem",
        utils::ConstructFtpUrl("", "", "example.com", "/files/demo.dem")
    );
}

TEST(FtpUtilsTest, NormalizeFtpUrlEncodesRawCredentials)
{
    EXPECT_EQ(
        "ftp://user%40mail.com:p%40ss%3Aword@example.com/files/demo.dem",
        utils::NormalizeFtpUrl("ftp://user@mail.com:p@ss:word@example.com/files/demo.dem")
    );
}

TEST(FtpUtilsTest, NormalizeFtpUrlDoesNotDoubleEncodeCredentials)
{
    EXPECT_EQ(
        "ftp://user%40mail.com:p%40ss%3Aword@example.com/files/demo.dem",
        utils::NormalizeFtpUrl("ftp://user%40mail.com:p%40ss%3Aword@example.com/files/demo.dem")
    );
}

TEST(FtpUtilsTest, HasWildcardPathLooksAtRemotePathOnly)
{
    EXPECT_TRUE(utils::HasWildcardPath("ftp://user:password@example.com/demos/hltv_demo_*.dem"));
    EXPECT_FALSE(utils::HasWildcardPath("ftp://user%40mail.com:password@example.com/demos/hltv_demo.dem"));
}

TEST(FtpUtilsTest, BuildDownloadPathUsesRemoteNameForDirectoryTargets)
{
    EXPECT_EQ(
        std::filesystem::path("demos") / "hltv_demo.dem",
        utils::BuildDownloadPath("demos/", "hltv_demo.dem")
    );

    EXPECT_EQ(
        std::filesystem::path("demos") / "hltv_demo.dem",
        utils::BuildDownloadPath("demos", "hltv_demo.dem", true)
    );
}

TEST(FtpUtilsTest, BuildDownloadPathPreservesExplicitFileTargets)
{
    EXPECT_EQ(
        std::filesystem::path("demos") / "latest.dem",
        utils::BuildDownloadPath("demos/latest.dem", "hltv_demo.dem")
    );
}
