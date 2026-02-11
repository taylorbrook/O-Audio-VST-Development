#include "VBAPDataExchange.h"

extern "C" {
#include "saf.h"
}

void VBAPComputeThread::run()
{
    while (! threadShouldExit())
    {
        wait (-1); // Wait for notification

        if (threadShouldExit())
            break;

        SpeakerLayout layout;
        bool shouldCompute = false;

        {
            const juce::ScopedLock lock (requestLock);
            if (hasRequest)
            {
                layout = pendingLayout;
                hasRequest = false;
                shouldCompute = true;
            }
        }

        if (shouldCompute)
            computeGainTable (layout);
    }
}

void VBAPComputeThread::computeGainTable (const SpeakerLayout& layout)
{
    int totalSpeakers = layout.getChannelCount();
    if (totalSpeakers < 4)
        return; // 2-3 speakers handled by pair-wise panning, no SAF needed

    // Build speaker direction array, excluding LFE
    std::vector<float> lsDirsDeg;
    std::vector<int> speakerToChannel;
    int numVBAPSpeakers = 0;

    for (int i = 0; i < totalSpeakers; ++i)
    {
        if (layout.speakers[(size_t) i].isLFE)
            continue;

        lsDirsDeg.push_back (layout.speakers[(size_t) i].azimuth);
        lsDirsDeg.push_back (layout.speakers[(size_t) i].elevation);
        speakerToChannel.push_back (i);
        ++numVBAPSpeakers;
    }

    if (numVBAPSpeakers < 4)
        return;

    float* gtable = nullptr;
    int nGtable = 0;
    int nTriangles = 0;

    int aziRes = 1;
    int elevRes = 1;

    if (layout.is3D)
    {
        generateVBAPgainTable3D (
            lsDirsDeg.data(),
            numVBAPSpeakers,
            aziRes,    // azimuth resolution
            elevRes,   // elevation resolution
            1,         // omit large triangles
            1,         // enable dummy speakers
            0.0f,      // spread
            &gtable,
            &nGtable,
            &nTriangles
        );
    }
    else
    {
        // 2D: all elevation = 0, use same function with enableDummies
        generateVBAPgainTable3D (
            lsDirsDeg.data(),
            numVBAPSpeakers,
            aziRes,
            elevRes,
            1,
            1,         // enableDummies adds polar speakers for convex hull
            0.0f,
            &gtable,
            &nGtable,
            &nTriangles
        );
    }

    if (gtable == nullptr || nGtable <= 0)
        return;

    auto data = std::make_unique<VBAPData>();
    data->gainTable.assign (gtable, gtable + (size_t) (nGtable * numVBAPSpeakers));
    data->speakerToChannelMap = speakerToChannel;
    data->numVBAPSpeakers = numVBAPSpeakers;
    data->numOutputChannels = totalSpeakers;
    data->aziRes = aziRes;
    data->elevRes = elevRes;
    data->is3D = layout.is3D;

    free (gtable);

    dataExchange.setNewData (std::move (data));
}
