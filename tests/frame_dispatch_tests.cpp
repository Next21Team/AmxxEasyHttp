#include <gtest/gtest.h>

#include "FramePump.h"
#include "easy_http/CompletionQueue.h"

using namespace ezhttp;

TEST(FramePumpTest, StartFrameWorksWithOrWithoutEngineNotifications)
{
    FramePump pump;
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::StartFrame));
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::StartFrame));
    EXPECT_FALSE(pump.ShouldDispatch(FrameSource::EngineFrameEnd));
    EXPECT_FALSE(pump.ShouldDispatch(FrameSource::EngineFrameEnd));
}

TEST(FramePumpTest, PauseInsideStartFrameKeepsOneDispatchPerEngineFrame)
{
    FramePump pump;
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::StartFrame));
    EXPECT_FALSE(pump.ShouldDispatch(FrameSource::PausedEngineFrameEnd));
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::PausedEngineFrameEnd));
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::PausedEngineFrameEnd));
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::StartFrame));
    EXPECT_FALSE(pump.ShouldDispatch(FrameSource::EngineFrameEnd));
    EXPECT_TRUE(pump.ShouldDispatch(FrameSource::PausedEngineFrameEnd));
}

TEST(CompletionQueueTest, MixedQueueSkipsDeferredEntriesAndResumesInCompletionOrder)
{
    CompletionQueue<int> queue;
    queue.Push(1, false);
    queue.Push(2, true);
    queue.Push(3, false);
    queue.Push(4, true);
    int value = 0;
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::OptedIn));
    EXPECT_EQ(value, 2);
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::OptedIn));
    EXPECT_EQ(value, 4);
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::OptedIn));
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::OptedIn));
    queue.Push(5, true);
    for (int expected : {1, 3, 5})
    {
        ASSERT_TRUE(queue.Pop(value, CallbackDelivery::All));
        EXPECT_EQ(value, expected);
    }
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::OptedIn));
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::All));
}

TEST(CompletionQueueTest, NormalFramesRetireOptedInEntriesFromBothViews)
{
    CompletionQueue<int> queue;
    queue.Push(1, true);
    queue.Push(2, false);
    queue.Push(3, true);
    int value = 0;
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::All));
    EXPECT_EQ(value, 1);
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::OptedIn));
    EXPECT_EQ(value, 3);
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::OptedIn));
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::All));
    EXPECT_EQ(value, 2);
    EXPECT_EQ(queue.size(), 0u);
}

TEST(CompletionQueueTest, SuspendingDeliveryPreservesBothKindsOfCompletion)
{
    CompletionQueue<int> queue;
    queue.Push(1, false);
    queue.Push(2, true);
    int value = 0;
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::Suspended));
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::OptedIn));
    EXPECT_EQ(value, 2);
    EXPECT_FALSE(queue.Pop(value, CallbackDelivery::Suspended));
    ASSERT_TRUE(queue.Pop(value, CallbackDelivery::All));
    EXPECT_EQ(value, 1);
}
