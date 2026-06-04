/*
 * CUBIC-FIT Congestion Control
 * Implementation based on TcpCubic in NS-3
 */

#include "tcp-cubic-fit.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpCubicFit");

NS_OBJECT_ENSURE_REGISTERED(TcpCubicFit);

TypeId
TcpCubicFit::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpCubicFit")
            .SetParent<TcpCongestionOps>()
            .AddConstructor<TcpCubicFit>()
            .SetGroupName("Internet")
            .AddAttribute("FastConvergence",
                          "Enable (true) or disable (false) fast convergence",
                          BooleanValue(true),
                          MakeBooleanAccessor(&TcpCubicFit::m_fastConvergence),
                          MakeBooleanChecker())
            .AddAttribute("TcpFriendliness",
                          "Enable (true) or disable (false) TCP friendliness",
                          BooleanValue(true),
                          MakeBooleanAccessor(&TcpCubicFit::m_tcpFriendliness),
                          MakeBooleanChecker())
            .AddAttribute("Beta",
                          "Beta for cubic multiplicative decrease",
                          DoubleValue(0.7),
                          MakeDoubleAccessor(&TcpCubicFit::m_beta),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("HyStart",
                          "Enable (true) or disable (false) hybrid slow start algorithm",
                          BooleanValue(true),
                          MakeBooleanAccessor(&TcpCubicFit::m_hystart),
                          MakeBooleanChecker())
            .AddAttribute("HyStartLowWindow",
                          "Lower bound cWnd for hybrid slow start (segments)",
                          UintegerValue(16),
                          MakeUintegerAccessor(&TcpCubicFit::m_hystartLowWindow),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("HyStartDetect",
                          "Hybrid Slow Start detection mechanisms: packet train, delay, both",
                          EnumValue(HybridSSDetectionMode::BOTH),
                          MakeEnumAccessor<HybridSSDetectionMode>(&TcpCubicFit::m_hystartDetect),
                          MakeEnumChecker(HybridSSDetectionMode::PACKET_TRAIN,
                                          "PACKET_TRAIN",
                                          HybridSSDetectionMode::DELAY,
                                          "DELAY",
                                          HybridSSDetectionMode::BOTH,
                                          "BOTH"))
            .AddAttribute("HyStartMinSamples",
                          "Number of delay samples for detecting the increase of delay",
                          UintegerValue(8),
                          MakeUintegerAccessor(&TcpCubicFit::m_hystartMinSamples),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("HyStartAckDelta",
                          "Spacing between ack's indicating train",
                          TimeValue(MilliSeconds(2)),
                          MakeTimeAccessor(&TcpCubicFit::m_hystartAckDelta),
                          MakeTimeChecker())
            .AddAttribute("HyStartDelayMin",
                          "Minimum time for hystart algorithm",
                          TimeValue(MilliSeconds(4)),
                          MakeTimeAccessor(&TcpCubicFit::m_hystartDelayMin),
                          MakeTimeChecker())
            .AddAttribute("HyStartDelayMax",
                          "Maximum time for hystart algorithm",
                          TimeValue(MilliSeconds(1000)),
                          MakeTimeAccessor(&TcpCubicFit::m_hystartDelayMax),
                          MakeTimeChecker())
            .AddAttribute("CubicDelta",
                          "Delta Time to wait after fast recovery before adjusting param",
                          TimeValue(MilliSeconds(10)),
                          MakeTimeAccessor(&TcpCubicFit::m_cubicDelta),
                          MakeTimeChecker())
            .AddAttribute("CntClamp",
                          "Counter value when no losses are detected",
                          UintegerValue(20),
                          MakeUintegerAccessor(&TcpCubicFit::m_cntClamp),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("C",
                          "Cubic Scaling factor",
                          DoubleValue(0.4),
                          MakeDoubleAccessor(&TcpCubicFit::m_c),
                          MakeDoubleChecker<double>(0.0))
            /* CUBIC-FIT Attributes */
            .AddAttribute("FitBeta",
                          "Tuning factor for RTT gradient impact on Alpha",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&TcpCubicFit::m_fitBeta),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("FitDelta",
                          "Tuning factor for queue prediction",
                          DoubleValue(0.1),
                          MakeDoubleAccessor(&TcpCubicFit::m_fitDelta),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("FitGamma",
                          "Smoothing factor for EWMA queue estimation",
                          DoubleValue(0.9),
                          MakeDoubleAccessor(&TcpCubicFit::m_fitGamma),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("UpdateInterval",
                          "Interval to update Alpha and N",
                          TimeValue(MilliSeconds(500)),
                          MakeTimeAccessor(&TcpCubicFit::m_updateInterval),
                          MakeTimeChecker())
            .AddAttribute("UseModifications",
                          "Toggle Plans 1-3 vs Original Paper CUBIC-FIT",
                          BooleanValue(true),
                          MakeBooleanAccessor(&TcpCubicFit::m_useModifications),
                          MakeBooleanChecker())
            .AddTraceSource("Alpha",
                            "Current Alpha value for Delay-based scaling",
                            MakeTraceSourceAccessor(&TcpCubicFit::m_alpha),
                            "ns3::TracedValueCallback::Double")
            .AddTraceSource("NFlows",
                            "Current Virtual N-Flows value",
                            MakeTraceSourceAccessor(&TcpCubicFit::m_nFlows),
                            "ns3::TracedValueCallback::Double")
            .AddTraceSource("QueueDelay",
                            "Estimated Bottleneck Queue Delay",
                            MakeTraceSourceAccessor(&TcpCubicFit::m_queueDelay),
                            "ns3::TracedValueCallback::Time");

    return tid;
}

TcpCubicFit::TcpCubicFit()
    : TcpCongestionOps(),
      m_cWndCnt(0),
      m_lastMaxCwnd(0),
      m_bicOriginPoint(0),
      m_bicK(0.0),
      m_delayMin(Time::Min()),
      m_epochStart(Time::Min()),
      m_found(false),
      m_roundStart(Time::Min()),
      m_endSeq(0),
      m_lastAck(Time::Min()),
      m_cubicDelta(Time::Min()),
      m_currRtt(Time::Min()),
      m_sampleCnt(0),
      m_alpha(0.1),
      m_nFlows(1.0),
      m_rttMax(Time::Min()),
      m_rttPrev(Time::Min()),
      m_rttGradient(0.0),
      m_queueDelay(Time::Min()),
      m_queueDelayPred(Time::Min()),
      m_queueDelaySmoothed(0.0),
      m_useModifications(true),
      m_lastUpdate(Time::Min())
{
    NS_LOG_FUNCTION(this);
}

TcpCubicFit::TcpCubicFit(const TcpCubicFit& sock)
    : TcpCongestionOps(sock),
      m_fastConvergence(sock.m_fastConvergence),
      m_beta(sock.m_beta),
      m_hystart(sock.m_hystart),
      m_hystartDetect(sock.m_hystartDetect),
      m_hystartLowWindow(sock.m_hystartLowWindow),
      m_hystartAckDelta(sock.m_hystartAckDelta),
      m_hystartDelayMin(sock.m_hystartDelayMin),
      m_hystartDelayMax(sock.m_hystartDelayMax),
      m_hystartMinSamples(sock.m_hystartMinSamples),
      m_initialCwnd(sock.m_initialCwnd),
      m_cntClamp(sock.m_cntClamp),
      m_c(sock.m_c),
      m_cWndCnt(sock.m_cWndCnt),
      m_lastMaxCwnd(sock.m_lastMaxCwnd),
      m_bicOriginPoint(sock.m_bicOriginPoint),
      m_bicK(sock.m_bicK),
      m_delayMin(sock.m_delayMin),
      m_epochStart(sock.m_epochStart),
      m_found(sock.m_found),
      m_roundStart(sock.m_roundStart),
      m_endSeq(sock.m_endSeq),
      m_lastAck(sock.m_lastAck),
      m_cubicDelta(sock.m_cubicDelta),
      m_currRtt(sock.m_currRtt),
      m_sampleCnt(sock.m_sampleCnt),
      m_alpha(sock.m_alpha),
      m_nFlows(sock.m_nFlows),
      m_rttMax(sock.m_rttMax),
      m_rttPrev(sock.m_rttPrev),
      m_rttGradient(sock.m_rttGradient),
      m_queueDelay(sock.m_queueDelay),
      m_queueDelayPred(sock.m_queueDelayPred),
      m_queueDelaySmoothed(sock.m_queueDelaySmoothed),
      m_useModifications(sock.m_useModifications),
      m_fitBeta(sock.m_fitBeta),
      m_fitGamma(sock.m_fitGamma),
      m_fitDelta(sock.m_fitDelta),
      m_updateInterval(sock.m_updateInterval),
      m_lastUpdate(sock.m_lastUpdate)
{
    NS_LOG_FUNCTION(this);
}

std::string
TcpCubicFit::GetName() const
{
    return "TcpCubicFit";
}

void
TcpCubicFit::Init(Ptr<TcpSocketState> tcb)
{
    HystartReset(tcb);
    m_lastUpdate = Simulator::Now();
}

void
TcpCubicFit::HystartReset(Ptr<const TcpSocketState> tcb)
{
    NS_LOG_FUNCTION(this);
    m_roundStart = m_lastAck = Simulator::Now();
    m_endSeq = tcb->m_highTxMark;
    m_currRtt = Time::Min();
    m_sampleCnt = 0;
}

void
TcpCubicFit::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (!tcb->m_isCwndLimited)
    {
        return;
    }

    // Slow Start
    if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        if (m_hystart && tcb->m_lastAckedSeq > m_endSeq)
        {
            HystartReset(tcb);
        }
        tcb->m_cWnd += segmentsAcked * tcb->m_segmentSize;
        segmentsAcked = 0;
    }

    // Congestion Avoidance
    if (tcb->m_cWnd >= tcb->m_ssThresh && segmentsAcked > 0)
    {
        m_cWndCnt += segmentsAcked;
        uint32_t cnt = Update(tcb, segmentsAcked);

        if (m_cWndCnt >= cnt)
        {
            tcb->m_cWnd += tcb->m_segmentSize;
            m_cWndCnt -= cnt;
        }
    }
}

uint32_t
TcpCubicFit::Update(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this);
    Time t;
    uint32_t delta;
    uint32_t bicTarget;
    uint32_t cnt = 0;
    uint32_t maxCnt;
    double offs;
    uint32_t segCwnd = tcb->GetCwndInSegments();

    m_ackCnt += segmentsAcked;

    // PLAN: Simulated Multi-Flow Aggregation - Scaling Cubic Coefficient by N^3
    // Use effectively scaled C based on N flows: C' = C * N^3
    double effectiveC = m_c * std::pow(m_nFlows, 3);
    // Ensure effectiveC is not zero
    if (effectiveC < 1e-9)
    {
        effectiveC = 1e-9;
    }

    if (m_epochStart == Time::Min())
    {
        m_epochStart = Simulator::Now();
        m_ackCnt = segmentsAcked;
        m_tcpCwnd = segCwnd;

        if (m_lastMaxCwnd <= segCwnd)
        {
            m_bicK = 0.0;
            m_bicOriginPoint = segCwnd;
        }
        else
        {
            // Calculate K using effective C
            // K = (Wmax - W) / C' ^ (1/3)
            m_bicK = std::pow((m_lastMaxCwnd - segCwnd) / effectiveC, 1.0 / 3.0);
            m_bicOriginPoint = m_lastMaxCwnd;
        }
    }

    t = Simulator::Now() + m_delayMin - m_epochStart;

    // Recalculate K if N changed significantly?
    // Ideally, we should maintain consistency.
    // If N changes, K should shift if we want to reach the SAME Wmax from SAME origin?
    // Current Logic: Re-evaluating K at start of epoch.
    // If N changes mid-epoch, our curve shape changes.
    // We will stick to using the m_bicK calculated at start of epoch,
    // but maybe we should scale it?
    // Note: If C changes to C', K should be K' such that C * K^3 = C' * K'^3 = Diff.
    // So K' = K * (C/C')^(1/3) = K * (C / (C*N^3))^(1/3) = K / N.
    // So we can compute 'currentK' dynamically.

    // Let's re-compute K_current based on current N and initial conditions if possible.
    // But we don't store initial conditions fully.
    // Simplified approach: Just use effectiveC and the elapsed time.
    // Note: This might cause jumps if N changes abruptly.
    // However, N changes slowly.

    // For smooth transition, let's just use the current logic with effectiveC.
    // Note: TcpCubic uses m_bicK stored.
    // If we use stored m_bicK calculated with OLD N (at epoch start),
    // and NEW effectiveC, the target will be: W = Origin + C' * (t - K)^3.
    // If K matched C', this is fine. If K is stale, it might be off.
    // To be precise, we need K consistent with C'.
    // If we assume K was correct for previous N, and we just updated N...
    // Let's trust that small changes in N won't break it, or we should update m_bicK.
    // Let's update m_bicK dynamically based on current segregated variables.
    // Actually, m_bicK is "time to reach Wmax".
    // If we simply scale C, we change the aggressiveness.
    // Let's stick to standard behavior: curve is defined by C and K.

    double currentK = m_bicK; // Using stored K. Ideally should be scaled if N changed.
    // If we assume K scales with 1/N.
    // Let's try to recalculate K based on current cwnd? No, that's Reno.

    // Proceed with stored m_bicK for now to ensure continuity of the epoch.
    // (Changing K mid-epoch might cause discontinuities in W).

    if (t.GetSeconds() < currentK)
    {
        offs = currentK - t.GetSeconds();
    }
    else
    {
        offs = t.GetSeconds() - currentK;
    }

    // delta = C' * offs^3
    delta = static_cast<uint32_t>(effectiveC * std::pow(offs, 3));

    if (t.GetSeconds() < currentK)
    {
        bicTarget = m_bicOriginPoint - delta;
    }
    else
    {
        bicTarget = m_bicOriginPoint + delta;
    }

    if (bicTarget > segCwnd)
    {
        cnt = segCwnd / (bicTarget - segCwnd);
    }
    else
    {
        cnt = 100 * segCwnd;
    }

    if (m_lastMaxCwnd == 0 && cnt > m_cntClamp)
    {
        cnt = m_cntClamp;
    }

    if (m_tcpFriendliness)
    {
        // Fit Friendliness? The paper mentions ensuring friendliness via Alpha.
        // Standard Cubic friendliness uses Reno-equivalent.
        // We will keep standard Reno friendliness check.
        auto scale = static_cast<uint32_t>(8 * (1024 + m_beta * 1024) / 3 / (1024 - m_beta * 1024));
        delta = (segCwnd * scale) >> 3;
        while (m_ackCnt > delta)
        {
            m_ackCnt -= delta;
            m_tcpCwnd++;
        }
        if (m_tcpCwnd > segCwnd)
        {
            delta = m_tcpCwnd - segCwnd;
            maxCnt = segCwnd / delta;
            if (cnt > maxCnt)
            {
                cnt = maxCnt;
            }
        }
    }

    return std::max(cnt, 2U);
}

void
TcpCubicFit::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);

    if (m_epochStart != Time::Min() && (Simulator::Now() - m_epochStart) < m_cubicDelta)
    {
        return;
    }

    if (m_delayMin == Time::Min() || m_delayMin > rtt)
    {
        m_delayMin = rtt;
    }

    // CUBIC-FIT Updates
    UpdateFitParameters(tcb, rtt);

    if (m_hystart && tcb->m_cWnd <= tcb->m_ssThresh &&
        tcb->m_cWnd >= m_hystartLowWindow * tcb->m_segmentSize)
    {
        HystartUpdate(tcb, rtt);
    }
}

void
TcpCubicFit::UpdateFitParameters(Ptr<TcpSocketState> tcb, const Time& rtt)
{
    Time now = Simulator::Now();

    // Track RTT Max
    if (m_rttMax == Time::Min() || rtt > m_rttMax)
    {
        m_rttMax = rtt;
    }

    // PLAN: Dynamic Gradient Alpha - Tracking RTT Gradient
    // Track Gradient
    if (m_rttPrev != Time::Min())
    {
        // We need dt between samples strictly?
        // Using sample interval is better.
        // Let's approximate gradient using variation between RTT samples.
        // Note: rtt is the measurement.
        // There is no timestamp for the previous sample explicitly, but we get calls sequentially.
        // We can just use difference / 1 usually?
        // Or better: gradient = (RTT_curr - RTT_prev) / delta_time
        // Taking delta_time as roughly RTT or using system time diff.
        // Let's use (rtt - m_rttPrev) / RTT_min (unitless gradient?) or per second?
        // Paper says "d(RTT)/dt".
        // Let's use dt = current RTT (sample spacing is roughly RTT).
        if (rtt.GetSeconds() > 0)
        {
            m_rttGradient = (rtt.GetSeconds() - m_rttPrev.GetSeconds()) / rtt.GetSeconds();
        }
    }
    m_rttPrev = rtt;

    // PLAN: Bottleneck Queue Estimation - Smart Virtual Scaling (Part 1: Queue Estimation)
    // EWMA for Queue Delay
    // q = RTT - RTT_min
    Time q = rtt - m_delayMin;
    if (q < Seconds(0))
    {
        q = Seconds(0);
    }

    if (m_useModifications)
    {
        // Modified CUBIC-FIT (Plans 1-3)
        // ----------------------------------

        // EWMA for Queue Delay
        if (m_queueDelaySmoothed == 0.0)
        {
            m_queueDelaySmoothed = q.GetSeconds();
        }
        else
        {
            m_queueDelaySmoothed =
                m_fitGamma * m_queueDelaySmoothed + (1 - m_fitGamma) * q.GetSeconds();
        }
        m_queueDelay = Seconds(m_queueDelaySmoothed);

        // Prediction of Queue Delay
        double q_pred_val = m_queueDelay.Get().GetSeconds() + m_fitDelta * m_rttGradient;
        if (q_pred_val < 0)
        {
            q_pred_val = 0;
        }
        m_queueDelayPred = Seconds(q_pred_val);
    }
    else
    {
        // Base CUBIC-FIT (Original Paper)
        // ----------------------------------
        // No EWMA, no prediction. Just raw queue delay.
        m_queueDelay = q;
        m_queueDelayPred = q;
    }

    // Periodic Update of Alpha and N
    if (now - m_lastUpdate >= m_updateInterval)
    {
        double delayRange = 0.0;
        double rttMaxSec = m_rttMax.GetSeconds();
        double rttMinSec = m_delayMin.GetSeconds();

        if (rttMaxSec > 0)
        {
            delayRange = (rttMaxSec - rttMinSec) / (2.0 * rttMaxSec);
        }

        // Report Eq 7: alpha = min(1/10, delayRange)
        m_alpha = std::min(0.1, delayRange);

        // Ensure strictly positive alpha to avoid division by zero
        if (m_alpha < 0.0001)
        {
            m_alpha = 0.0001;
        }

        double rttSec = rtt.GetSeconds();
        if (rttSec > 0)
        {
            // Eq 6: N(t+1) = max(1, N(t) + 1 - ( (RTT_t - RTT_min) / (alpha * RTT_t) ) * N(t) )
            // For base formula: queueTerm is raw q. For modified: queueTerm relies on predicted Q
            // (or smoothed).
            double queueTerm;
            if (m_useModifications)
            {
                queueTerm = m_queueDelayPred.GetSeconds(); // Use Predicted/Smoothed Q
            }
            else
            {
                queueTerm = q.GetSeconds(); // Use raw Q
            }

            double denominator = m_alpha * rttSec;

            double ratio = 0.0;
            if (denominator > 0)
            {
                ratio = queueTerm / denominator;
            }

            // N(t+1) = N(t) + 1 - ratio * N(t)
            double nextN = m_nFlows + 1.0 - (ratio * m_nFlows);

            m_nFlows = std::max(1.0, nextN);
        }

        if (m_useModifications)
        {
            // Cap N to avoid extreme aggression in high jitter (Extension for stability)
            if (m_nFlows > 20.0)
            {
                m_nFlows = 20.0;
            }
        }

        m_lastUpdate = now;

        NS_LOG_INFO("Time=" << now.GetSeconds() << " RTT=" << rttSec << " MinRTT=" << rttMinSec
                            << " MaxRTT=" << rttMaxSec << " Alpha=" << m_alpha
                            << " N=" << m_nFlows);
    }
}

void
TcpCubicFit::HystartUpdate(Ptr<TcpSocketState> tcb, const Time& delay)
{
    NS_LOG_FUNCTION(this << delay);

    if (!m_found)
    {
        Time now = Simulator::Now();

        if ((now - m_lastAck) <= m_hystartAckDelta)
        {
            m_lastAck = now;
            if ((now - m_roundStart) > m_delayMin)
            {
                if (m_hystartDetect == HybridSSDetectionMode::PACKET_TRAIN ||
                    m_hystartDetect == HybridSSDetectionMode::BOTH)
                {
                    m_found = true;
                }
            }
        }

        if (m_sampleCnt < m_hystartMinSamples)
        {
            if (m_currRtt == Time::Min() || m_currRtt > delay)
            {
                m_currRtt = delay;
            }
            ++m_sampleCnt;
        }
        else if (m_currRtt > m_delayMin + HystartDelayThresh(m_delayMin))
        {
            if (m_hystartDetect == HybridSSDetectionMode::DELAY ||
                m_hystartDetect == HybridSSDetectionMode::BOTH)
            {
                m_found = true;
            }
        }

        if (m_found)
        {
            tcb->m_ssThresh = tcb->m_cWnd;
        }
    }
}

Time
TcpCubicFit::HystartDelayThresh(const Time& t) const
{
    Time ret = t;
    if (t > m_hystartDelayMax)
    {
        ret = m_hystartDelayMax;
    }
    else if (t < m_hystartDelayMin)
    {
        ret = m_hystartDelayMin;
    }
    return ret;
}

uint32_t
TcpCubicFit::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    uint32_t segCwnd = tcb->GetCwndInSegments();

    if (segCwnd < m_lastMaxCwnd && m_fastConvergence)
    {
        m_lastMaxCwnd = (segCwnd * (1 + m_beta)) / 2;
    }
    else
    {
        m_lastMaxCwnd = segCwnd;
    }

    m_epochStart = Time::Min();

    uint32_t ssThresh = std::max(static_cast<uint32_t>(segCwnd * m_beta), 2U) * tcb->m_segmentSize;
    return ssThresh;
}

void
TcpCubicFit::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                const TcpSocketState::TcpCongState_t newState)
{
    if (newState == TcpSocketState::CA_LOSS)
    {
        CubicReset(tcb);
        HystartReset(tcb);
    }
}

void
TcpCubicFit::CubicReset(Ptr<const TcpSocketState> tcb)
{
    m_bicOriginPoint = 0;
    m_bicK = 0;
    m_ackCnt = 0;
    m_tcpCwnd = 0;
    m_delayMin = Time::Min();
    m_found = false;
    // Reset RTT max? No, retain long-term history
}

Ptr<TcpCongestionOps>
TcpCubicFit::Fork()
{
    return CopyObject<TcpCubicFit>(this);
}

} // namespace ns3
