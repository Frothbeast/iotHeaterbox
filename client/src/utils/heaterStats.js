const StatsLib = {
  avg: (arr) => arr.length ? (arr.reduce((a, b) => a + b, 0) / arr.length) : 0,
  max: (arr) => arr.length ? Math.max(...arr) : 0,
  min: (arr) => arr.length ? Math.min(...arr) : 0,
};

export const calculateColumnStats = (heaterRecords) => {
  if (!heaterRecords?.length) return null;

  const tempBoxs = heaterRecords.map(r => parseFloat(r.tempBox)).filter(v => !isNaN(v));
  const tempHeaters = heaterRecords.map(r => parseFloat(r.tempHeater)).filter(v => !isNaN(v));
  const rssiAvgs = heaterRecords.map(r => parseInt(r.rssi)).filter(v => !isNaN(v));
  const lastRecord = heaterRecords[0];
  const dateObj = new Date(lastRecord.datetime);

  return {
    lastTime: dateObj.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit', hour12: true }),
    lastDate: dateObj.toLocaleDateString(),
    lastBoxTemp: lastRecord.tempBox,
    lastHeaterTemp: lastRecord.tempHeater,
    lastHeater: lastRecord.heater,
    lastRSSI: lastRecord.rssiAvg
  };
};