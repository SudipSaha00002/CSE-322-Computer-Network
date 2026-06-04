/*
 * CUBIC-FIT Congestion Control
 * Based on TcpCubic implementation in NS-3
 */

#ifndef TCPCUBICFIT_H
#define TCPCUBICFIT_H

#include "tcp-congestion-ops.h"
#include "tcp-socket-base.h"

#include "ns3/nstime.h"
#include "ns3/traced-value.h"

#include <string>

namespace ns3
{

/**
 * @brief The CUBIC-FIT Congestion Control Algorithm
 *
 * CUBIC-FIT extends CUBIC with:
 * 1. Dynamic Alpha (α) Adaptation based on RTT gradient.
 * 2. Bottleneck Queue Estimation for smart virtual scaling.
 * 3. Simulated Multi-Flow Aggregation (N flows).
 */
class TcpCubicFit : public TcpCongestionOps
{
  public:
    /**
     * @brief Values to detect the Slow Start mode of HyStart (inherited concept)
     */
    enum HybridSSDetectionMode
    {
        PACKET_TRAIN = 1, //!< Detection by trains of packet
        DELAY = 2,        //!< Detection by delay value
        BOTH = 3,         //!< Detection by both
    };

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TcpCubicFit();

    /**
     * Copy constructor
     * @param sock Socket to copy
     */
    TcpCubicFit(const TcpCubicFit& sock);

    std::string GetName() const override;
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;

    Ptr<TcpCongestionOps> Fork() override;
    void Init(Ptr<TcpSocketState> tcb) override;

  private:
    /* Standard Cubic Parameters (cloned from TcpCubic) */
    bool m_fastConvergence; //!< Enable or disable fast convergence algorithm
    bool m_tcpFriendliness; //!< Enable or disable TCP-friendliness heuristic
    double m_beta;          //!< Beta for cubic multiplicative increase

    bool m_hystart;                        //!< Enable or disable HyStart algorithm
    HybridSSDetectionMode m_hystartDetect; //!< Detect way for HyStart algorithm
    uint32_t m_hystartLowWindow;           //!< Lower bound cWnd for hybrid slow start (segments)
    Time m_hystartAckDelta;                //!< Spacing between ack's indicating train
    Time m_hystartDelayMin;                //!< Minimum time for hystart algorithm
    Time m_hystartDelayMax;                //!< Maximum time for hystart algorithm
    uint8_t m_hystartMinSamples; //!< Number of delay samples for detecting the increase of delay

    uint32_t m_initialCwnd; //!< Initial cWnd
    uint8_t m_cntClamp;     //!< Modulo of the (avoided) float division for cWnd

    double m_c; //!< Cubic Scaling factor

    // Cubic parameters
    uint32_t m_cWndCnt;        //!<  cWnd integer-to-float counter
    uint32_t m_lastMaxCwnd;    //!<  Last maximum cWnd
    uint32_t m_bicOriginPoint; //!<  Origin point of bic function
    double m_bicK;             //!<  Time to origin point from the beginning
                               //    of the current epoch (in s)
    Time m_delayMin;           //!<  Min delay
    Time m_epochStart;         //!<  Beginning of an epoch
    bool m_found;              //!<  The exit point is found?
    Time m_roundStart;         //!<  Beginning of each round
    SequenceNumber32 m_endSeq; //!<  End sequence of the round
    Time m_lastAck;            //!<  Last time when the ACK spacing is close
    Time m_cubicDelta;         //!<  Time to wait after recovery before update
    Time m_currRtt;            //!<  Current Rtt
    uint32_t m_sampleCnt;      //!<  Count of samples for HyStart
    uint32_t m_ackCnt;         //!<  Count the number of ACKed packets
    uint32_t m_tcpCwnd;        //!<  Estimated tcp cwnd (for Reno-friendliness)

    // CUBIC-FIT Specific Parameters
    // PLAN: Dynamic Gradient Alpha - Alpha Parameter
    TracedValue<double> m_alpha; //!< Dynamic adaptation parameter
    // PLAN: Simulated Multi-Flow Aggregation - Virtual Flow Count (N)
    TracedValue<double> m_nFlows; //!< Virtual number of flows (N)

    bool m_useModifications; //!< Toggle for Plans 1-3 vs Original Paper

    // PLAN: Dynamic Gradient Alpha - RTT Gradient Tracking
    // RTT Tracking for Gradient
    Time m_rttMax;        //!< Max RTT observed
    Time m_rttPrev;       //!< Previous RTT sample for gradient
    double m_rttGradient; //!< Current RTT gradient (slope) (s/s)

    // PLAN: Bottleneck Queue Estimation - Queue Delay Variables
    // Queue Estimation
    TracedValue<Time> m_queueDelay; //!< Estimated queuing delay (q)
    Time m_queueDelayPred;          //!< Predicted queuing delay estimate
    double m_queueDelaySmoothed;    //!< EWMA of queue delay (in seconds)

    // Tuning Parameters
    double m_fitBeta;      //!< Beta factor for alpha gradient adjustment
    double m_fitGamma;     //!< Gamma factor for EWMA (queue estimation)
    double m_fitDelta;     //!< Delta factor for queue prediction
    Time m_updateInterval; //!< Interval to update Alpha and N
    Time m_lastUpdate;     //!< Last time Alpha/N were updated

  private:
    /**
     * @brief Reset HyStart parameters
     * @param tcb Transmission Control Block of the connection
     */
    void HystartReset(Ptr<const TcpSocketState> tcb);

    /**
     * @brief Reset Cubic parameters
     * @param tcb Transmission Control Block of the connection
     */
    void CubicReset(Ptr<const TcpSocketState> tcb);

    /**
     * @brief Cubic window update after a new ack received
     * @param tcb Transmission Control Block of the connection
     * @param segmentsAcked Segments acked
     * @returns the congestion window update counter
     */
    uint32_t Update(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

    /**
     * @brief Update HyStart parameters
     *
     * @param tcb Transmission Control Block of the connection
     * @param delay Delay for HyStart algorithm
     */
    void HystartUpdate(Ptr<TcpSocketState> tcb, const Time& delay);

    /**
     * @brief Clamp time value in a range
     *
     * The returned value is t, clamped in a range specified
     * by attributes (HystartDelayMin < t < HystartDelayMax)
     *
     * @param t Time value to clamp
     * @return t itself if it is in range, otherwise the min or max
     * value
     */
    Time HystartDelayThresh(const Time& t) const;

    /**
     * @brief Update CUBIC-FIT parameters (Alpha, N)
     * @param tcb The socket state
     * @param rtt The current RTT sample
     */
    void UpdateFitParameters(Ptr<TcpSocketState> tcb, const Time& rtt);

    /**
     * @brief Calculate the cubic root
     * @param x The value
     * @return The cubic root of x
     */
    double CubicRoot(double x) const;
};

} // namespace ns3

#endif // TCPCUBICFIT_H
