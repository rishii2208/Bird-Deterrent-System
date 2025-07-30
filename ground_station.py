

import sys
import json
import time
import logging
import sqlite3
import threading
from datetime import datetime, timedelta
from dataclasses import dataclass, asdict
from typing import Dict, List, Optional, Tuple
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

try:
    from PyQt5.QtWidgets import *
    from PyQt5.QtCore import *
    from PyQt5.QtGui import *
    import serial
    import requests
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install PyQt5 matplotlib numpy pyserial requests")
    sys.exit(1)

CONFIG = {
    'serial_port': '/dev/ttyUSB0',
    'baud_rate': 115200,
    'update_interval': 1000,  
    'log_file': 'bird_deterrent.log',
    'database_file': 'telemetry.db',
    'backup_interval': 3600,  # seconds
    'alert_thresholds': {
        'battery_low': 11.0,
        'temperature_high': 60.0,
        'bird_close': 10.0,  
        'system_failure': True
    }
}

@dataclass
class TelemetryData:
    """Structure for telemetry data from drone"""
    timestamp: float
    state: str
    battery_voltage: float
    temperature: float
    bird_count: int
    closest_bird_distance: float
    weather_status: str
    system_health: str
    latitude: float = 0.0
    longitude: float = 0.0
    altitude: float = 0.0
    heading: float = 0.0

@dataclass
class SystemStatus:
   
    connected: bool = False
    last_update: float = 0.0
    alerts: List[str] = None
    mission_active: bool = False
    deterrent_activations: int = 0
    total_birds_deterred: int = 0
    
    def __post_init__(self):
        if self.alerts is None:
            self.alerts = []

class DatabaseManager:
   
    
    def __init__(self, db_file: str):
        self.db_file = db_file
        self.init_database()
    
    def init_database(self):
     
        conn = sqlite3.connect(self.db_file)
        cursor = conn.cursor()
        
       
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS telemetry (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp REAL,
                state TEXT,
                battery_voltage REAL,
                temperature REAL,
                bird_count INTEGER,
                closest_bird_distance REAL,
                weather_status TEXT,
                system_health TEXT,
                latitude REAL,
                longitude REAL,
                altitude REAL,
                heading REAL
            )
        ''')
        
       
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp REAL,
                event_type TEXT,
                description TEXT,
                severity TEXT
            )
        ''')
        
      
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS missions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                start_time REAL,
                end_time REAL,
                mission_type TEXT,
                total_birds_detected INTEGER,
                deterrent_activations INTEGER,
                success_rate REAL,
                notes TEXT
            )
        ''')
        
        conn.commit()
        conn.close()
    
    def insert_telemetry(self, data: TelemetryData):
      
        conn = sqlite3.connect(self.db_file)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO telemetry (
                timestamp, state, battery_voltage, temperature,
                bird_count, closest_bird_distance, weather_status,
                system_health, latitude, longitude, altitude, heading
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            data.timestamp, data.state, data.battery_voltage, data.temperature,
            data.bird_count, data.closest_bird_distance, data.weather_status,
            data.system_health, data.latitude, data.longitude, data.altitude, data.heading
        ))
        
        conn.commit()
        conn.close()
    
    def get_recent_telemetry(self, hours: int = 24)