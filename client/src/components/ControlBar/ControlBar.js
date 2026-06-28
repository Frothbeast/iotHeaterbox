import HeaterChart from '../HeaterTable/HeaterChart';
import './ControlBar.css';
import React, { useState } from 'react';

const ControlBar = ({ cl1pClick, selectedHours, onHoursChange, columnStats, records, toggleSidebar, isSidebarOpen, serverTime }) => {
  
  const sendHexCommand = async (hex) => {
    try {
      const response = await fetch(`${process.env.REACT_APP_HEATERBOX_API_URL}/api/send-command`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({hex})
      });
      if (!response.ok) throw new Error('Command failed');
      console.log("Command sent successfully");
    } catch (error) {
      console.error("Error:", error);
    }
  };

  const getOptions = (min, max) => ({
    responsive: true,
    maintainAspectRatio: false,
    plugins: { legend: { display: false } },
    scales: {
      x: { display: false, reverse: true },
      y: { display: false, min, max }
    },
    elements: { point: { radius: 0, hitRadius: 0, hoverRadius: 0 } }
  });
  
  const [newsetpoint, setNewsetpoint] = useState(40);

  return (
    <header className="controlBar">
      <div className="brandSection">
        <div className="brand">Heater</div>
        <div className="serverTime">
          <span className="stLabel">Server Time:</span>
          <span>{serverTime ?? "00:00:00"}</span>
        </div>
      </div>
      
      <div className="centerSection">
        <div className="lastRun">
          <span className="label">Last Reading</span>
          <span className="value">{columnStats?.lastTime ?? "N/a"}</span>
        </div>
        <div className="lastTemp">
          <span className="label">SP</span>
          <span className="value">{columnStats?.setpoint ?? "N/a"}</span>
          <span className="unit">°C</span>
          <span className="label">B</span>
          <span className="value">{columnStats?.lastBoxTemp ?? "N/a"}</span>
          <span className="unit">°C</span>
          <span className="label">H</span>
          <span className="value">{columnStats?.lastHeaterTemp ?? "N/a"}</span>
          <span className="unit">°C</span>
        </div>
        <div className="buttonRow">
          <button className="setpointup myBUTTon" onClick={() => setNewsetpoint(prev => prev + 1)}>+</button>
          <button className="setpointup myBUTTon" onClick={() => setNewsetpoint(prev => prev - 1)}>-</button>
          <div className="setpoint">{newsetpoint}</div>
          <button className="setpointsend myBUTTon" onClick={() => sendHexCommand(newsetpoint.toString(10).padStart(3, '0'))}>Send</button>
          
        </div>
        <div className="buttonRow">
          <button className="sidebarButton myBUTTon" onClick={toggleSidebar}>
            {isSidebarOpen ? "Table" : "Graph"}
          </button>
          <button onClick={cl1pClick} className="cl1pButton myBUTTon">CL1P</button>
          <select className="selectedHours myBUTTon" value={selectedHours} onChange={(e) => onHoursChange(Number(e.target.value))}>
            <option value={1}>1 Hour</option>
            <option value={8}>8 Hour</option>
            <option value={24}>24 Hour</option>
          </select>
        </div>
      </div>

      <div className="chartSection">
        <div className="chartContainer">
          <div className="chartWatermark">TEMP</div>
          <HeaterChart
            labels={records.map((_, i) => i)}
            datasets={[
              { label: "Setpoint", color: "white", data: records.map(r => r.setpoint) },
              { label: "Box", color: "red", data: records.map(r => r.tempBox) },
              { label: "Heater", color: "pink", data: records.map(r => r.tempHeater) }
            ]}
            options={getOptions(-10, 150)}
          />
        </div>
        <div className="chartContainer">
          <div className="chartWatermark">RSSI</div>
          <HeaterChart
            labels={records.map((_, i) => i)}
            datasets={[
              { label: "RSSI", color: "cyan", data: records.map(r => r.rssi) }
            ]}
            options={getOptions(0, 100)}
          />
        </div>
      </div>
    </header>
  );
};

export default ControlBar;