#include "SafeQueue.h"
#include "SocketUtils.h"
#include "common/Check.h"
#include "common/Logger.h"
#include "common/MemoryPool.h"
#include "common/ThreadUtils.h"
#include "common/TimeUtils.h"

#include <fstream>
#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <string>

void MyFunction(int firstArgument)
{
    SHOWINFO(firstArgument);
    while (firstArgument-- > 0)
        ;
}

TEST(Basic, ThreadExample)
{
    auto thread1 = CreateAndStartThread(1, "MyFunctionAffinity", MyFunction, 100);
    auto thread2 = CreateAndStartThread(-1, "MyFunctionNoAffinity", MyFunction, 25);

    thread1->join();
    thread2->join();
}

TEST(Basic, MemoryPoolExample)
{
    struct MyStruct
    {
        int d[3];

        MyStruct() = default;
        MyStruct(int a, int b, int c)
        {
            d[0] = a;
            d[1] = b;
            d[2] = c;
        }
    };

    MemoryPool<f64> primPool(50);
    MemoryPool<MyStruct> structPool(50);

    for (u32 i = 0; i < 50; ++i)
    {
        double *elem1 = primPool.Allocate((f64)i * 3);
        MyStruct *elem2 = structPool.Allocate(i * 3, i * 3 + 1, i * 3 + 2);

        SHOWINFO("elem1: ", *elem1, " allocated at: ", elem1);
        SHOWINFO("elem2: {", elem2->d[0], ", ", elem2->d[1], ", ", elem2->d[2], "} allocated at: ", elem2);

        if (i % 5 == 0)
        {
            SHOWINFO("Deallocating elem1: ", *elem1, " from: ", elem1);
            SHOWINFO("Deallocating elem2: {", elem2->d[0], ", ", elem2->d[1], ", ", elem2->d[2], "} from: ", elem2);

            primPool.Deallocate(elem1);
            structPool.Deallocate(elem2);
        }
    }
}

TEST(Basic, SafeQueueExample)
{
    struct MyStruct
    {
        f64 values[3];
    };
    SafeQueue<MyStruct> lfss(50);
    std::atomic<bool> shouldStop = false;
    auto consumeFunction = [&] {
        while (!shouldStop || lfss.GetSize() != 0)
        {
            if (lfss.GetSize() != 0)
            {
                MyStruct const *ms = lfss.GetNextRead();
                SHOWINFO("Reading from the queue values: {", ms->values[0], ", ", ms->values[1], ", ", ms->values[2],
                         "}")
                lfss.UpdateReadIndex();
            }
        }
    };

    auto ct = CreateAndStartThread(-1, "Consumer", consumeFunction);
    for (u32 i = 0; i < 50; ++i)
    {
        MyStruct *ms = lfss.GetNextWriteTo();

        ms->values[0] = i * 3 + 0;
        ms->values[1] = i * 3 + 1;
        ms->values[2] = i * 3 + 2;

        SHOWINFO("Writing to the queue values: {", ms->values[0], ", ", ms->values[1], ", ", ms->values[2], "}")
        lfss.UpdateWriteIndex();
    }
    shouldStop = true;
    ct->join();
}

TEST(Basic, Time)
{
    std::string currentTime;
    GetCurrentTimeStr(currentTime);
    SHOWINFO("Current time as string: ", currentTime);
    SHOWINFO(GetCurrentNanos());
}

TEST(Basic, Logger)
{
    {
        QuickLogger logger("output.txt");
        logger.Log("My logger is fancy ", 5, ' ', 'd', '\n');
    }
    {
        std::string expectedString = "My logger is fancy 5 d\n";
        std::string actualString;
        actualString.resize(expectedString.size());
        std::ifstream fin("output.txt");
        EXPECT_TRUE(fin.is_open());
        fin.read(actualString.data(), actualString.size());
        EXPECT_TRUE(fin.good());
        EXPECT_EQ(expectedString, actualString);
    }
}

TEST(Basic, SocketUtils)
{
    SHOWINFO(GetIFaceIP("ens160"));
}
