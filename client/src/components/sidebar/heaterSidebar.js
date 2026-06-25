import React, { useMemo } from 'react';
import HeaterChart from '../HeaterTable/HeaterChart'; 
import './sidebar.css';
import { Chart as ChartJS, registerables } from 'chart.js';
import 'chartjs-adapter-date-fns';
import zoomPlugin from 'chartjs-plugin-zoom';

ChartJS.register(...registerables, zoomPlugin);

const HeaterSidebar = ({ isOpen, records, selectedHours }) => {
  const timeUnit = selectedHours <= 1 ? 'minute' : (selectedHours <= 48 ? 'hour' : 'day');

  const createConfig = (unit) => ({
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        display: true,
        position: 'top',
        labels: { boxWidth: 20, font: { size: 14 }, color: 'lightgrey' }
      },
      zoom: {
        pan: { enabled: true, mode: 'x' },
        zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' }
      }
    },
    scales: {
      x: {
        type: 'time',
        time: { unit: unit },
        ticks: { color: 'lightgrey' },
        grid: { color: 'rgba(255,255,255,0.1)' }
      },
      y: {
        ticks: { color: 'lightgrey' },
        grid: { color: 'rgba(255,255,255,0.1)' }
      }
    }
  });

  const config = useMemo(() => createConfig(timeUnit), [timeUnit]);
  const labels = records.map(r => new Date(r.datetime));

  return (
    <div className={`sidebar ${isOpen ? 'open' : ''}`}>
      <div className="sidebarContent">
        <div className="chartContainer">
          <HeaterChart
            labels={labels}
            datasets={[
              { label: "Setpoint", color: "white", data: records.map(r => r.setpoint) },
              { label: "Box °C", color: "red", data: records.map(r => r.tempBox) },
              { label: "Heater °C", color: "pink", data: records.map(r => r.tempHeater) }
            ]}
            options={config}
          />
        </div>

        <div className="chartContainer">
          <HeaterChart
            labels={labels}
            datasets={[
              { label: "RSSI %", color: "cyan", data: records.map(r => r.rssiAvg) }
            ]}
            options={config}
          />
        </div>
      </div>
    </div>
  );
};

export default HeaterSidebar;