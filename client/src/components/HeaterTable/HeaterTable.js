import React from 'react';
import './HeaterTable.css';

const HeaterTable = ({ records = [], columnStats }) => {
    if (!columnStats) return null;

    return (
        <div className="heaterTableContainer">
            <table className="heaterTable">
                <thead className="heaterTableHeader">
                    <tr className="heaterTableHeaderRow1">
                        <th className="heaterTableHeaderCell1Row2">Time</th>
                        <th className="heaterTableHeaderCellRow1 smaller">Setpoint</th>
                        <th className="heaterTableHeaderCellRow1 smaller">Box °C</th>
                        <th className="heaterTableHeaderCellRow1 smaller">Heater °C</th>
                        <th className="heaterTableHeaderCellRow1 smaller">Status</th>
                        <th className="heaterTableHeaderCellRow1 smaller">RSSI</th>
                    </tr>
                </thead>
                <tbody className="heaterTableBody">
                    {records.map((record) => (
                        <tr key={record.id} className="heaterTableRow">
                            <td className="heaterTableCell2">
                                {new Date(record.datetime).toLocaleTimeString([], { hour: 'numeric', minute: '2-digit', hour12: true })}
                            </td>
                            <td className="heaterTableCell">{record.setpoint}</td>
                            <td className="heaterTableCell">{record.tempBox}</td>
                            <td className="heaterTableCell">{record.tempHeater}</td>
                            <td className="heaterTableCell">F:{record.fan} L:{record.light} H:{record.heater}</td>
                            <td className="heaterTableCell">{record.rssi}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
};

export default HeaterTable;