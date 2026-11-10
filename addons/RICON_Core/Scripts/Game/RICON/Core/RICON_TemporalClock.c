//! STAGING: CT10 clock arithmetic only. Enforce compile NOT_RUN; no engine/CF/maps/float clocks.
class RICON_TemporalClock
{
	//! 0 OK; 1 CLOCK_UNVERIFIED; 2 CLOCK_OUT_OF_RANGE. All outputs are (nonnegative ms, 0..999 us).
	static int MapInterval(int nowMs, int subUs, int localAnchorMs, int serverAnchorMs,
		int validFromLocalMs, int validUntilLocalMs, int maxErrorUs,
		out int estimateMs, out int estimateSubUs, out int lowerMs, out int lowerSubUs,
		out int upperMs, out int upperSubUs)
	{
		estimateMs = 0; estimateSubUs = 0;
		lowerMs = 0; lowerSubUs = 0; upperMs = 0; upperSubUs = 0;
		if (nowMs < 0 || subUs < 0 || subUs > 999 || localAnchorMs < 0 || serverAnchorMs < 0
			|| validFromLocalMs < 0 || validUntilLocalMs < 0 || maxErrorUs < 0
			|| validFromLocalMs > localAnchorMs || localAnchorMs >= validUntilLocalMs
			|| nowMs < validFromLocalMs || nowMs >= validUntilLocalMs)
			return 1;
		// Two nonnegative int32 times differ within [-INT_MAX, INT_MAX], never INT_MIN.
		int delta = nowMs - localAnchorMs;
		if (delta >= 0)
		{
			if (serverAnchorMs > int.MAX - delta)
				return 2;
		}
		else if (serverAnchorMs < -delta)
			return 2;
		int mappedMs = serverAnchorMs + delta;
		int errorMs = maxErrorUs / 1000;
		int errorSubUs = maxErrorUs % 1000;
		int subtractMs = errorMs;
		int lowSub = subUs - errorSubUs;
		if (lowSub < 0)
		{
			lowSub += 1000;
			subtractMs += 1;
		}
		int addMs = errorMs;
		int highSub = subUs + errorSubUs;
		if (highSub >= 1000)
		{
			highSub -= 1000;
			addMs += 1;
		}
		if (mappedMs < subtractMs || mappedMs > int.MAX - addMs)
			return 2;
		estimateMs = mappedMs; estimateSubUs = subUs;
		lowerMs = mappedMs - subtractMs; lowerSubUs = lowSub;
		upperMs = mappedMs + addMs; upperSubUs = highSub;
		return 0;
	}

	//! Physical [from, until) excludes every subUs at upperMs == until, including zero.
	static bool ContainsInterval(int lowerMs, int lowerSubUs, int upperMs, int upperSubUs,
		int validFromMs, int validUntilMs)
	{
		if (lowerMs < 0 || upperMs < 0 || lowerSubUs < 0 || lowerSubUs > 999
			|| upperSubUs < 0 || upperSubUs > 999 || validFromMs < 0 || validUntilMs <= validFromMs
			|| lowerMs > upperMs || (lowerMs == upperMs && lowerSubUs > upperSubUs))
			return false;
		return lowerMs >= validFromMs && upperMs < validUntilMs;
	}
}
