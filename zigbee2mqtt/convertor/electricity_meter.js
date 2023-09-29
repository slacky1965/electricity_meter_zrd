const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const ota = require('zigbee-herdsman-converters/lib/ota');
const extend = require('zigbee-herdsman-converters/lib/extend');
const utils = require('zigbee-herdsman-converters/lib/utils');
const globalStore = require('zigbee-herdsman-converters/lib/store');
const { postfixWithEndpointName, precisionRound } = require('zigbee-herdsman-converters/lib/utils.js') 
const e = exposes.presets;
const ea = exposes.access;

const switchDeviceModel = ['No Device', 'KASKAD-1-MT', 'KASKAD-11-C1', 'MERCURY-206', 'ENERGOMERA-CE102M'];

const tzLocal = {
  device_address_config: {
    key: ['device_address_preset'],
      convertSet: async (entity, key, rawValue, meta) => {
			  const endpoint = meta.device.getEndpoint(1);
        const lookup = {'OFF': 0x00, 'ON': 0x01};
        const value = lookup.hasOwnProperty(rawValue) ? lookup[rawValue] : parseInt(rawValue, 10);
        const payloads = {
				  device_address_preset: ['seMetering', {0xf001: {value, type: 0x23}}],
        };
        await endpoint.write(payloads[key][0], payloads[key][1]);
        return {
          state: {[key]: rawValue},
        };
      },
  },
  device_model_config: {
    key: ['device_model_preset'],
      convertSet: async (entity, key, rawValue, meta) => {
			  const endpoint = meta.device.getEndpoint(1);
        const value = switchDeviceModel.indexOf(rawValue);
        const payloads = {
          device_model_preset: ['seMetering', {0xf000: {value, type: 0x30}}],
        };
        await endpoint.write(payloads[key][0], payloads[key][1]);
        return {
          state: {[key]: rawValue},
        };
      },
  },
  device_measurement_config: {
    key: ['device_measurement_preset'],
      convertSet: async (entity, key, rawValue, meta) => {
			  const endpoint = meta.device.getEndpoint(1);
        const lookup = {'OFF': 0x00, 'ON': 0x01};
        const value = lookup.hasOwnProperty(rawValue) ? lookup[rawValue] : parseInt(rawValue, 10);
        const payloads = {
				  device_measurement_preset: ['seMetering', {0xf002: {value, type: 0x20}}],
        };
        await endpoint.write(payloads[key][0], payloads[key][1]);
        return {
          state: {[key]: rawValue},
        };
      },
  },
  metering: {
    key:['tariff'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('seMetering', ['currentTier1SummDelivered', 'currentTier2SummDelivered', 'currentTier3SummDelivered', 'currentTier4SummDelivered', 'multiplier', 'divisor']);
        },
	    convertSet: async (entity, key, value, meta) => {
	        return null;
	    },
    key:['serial_number'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('seMetering', ['meterSerialNumber']);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
    key:['date_release'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('seMetering', [0xf003]);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
    key:['model_name'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('seMetering', [0xf004]);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
    key:['voltage'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('haElectricalMeasurement', ['rmsVoltage', 'acVoltageMultiplier', 'acVoltageDivisor']);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
    key:['current'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('haElectricalMeasurement', ['instantaneousLineCurrent', 'acCurrentMultiplier', 'acCurrentDivisor']);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
    key:['power'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('haElectricalMeasurement', ['apparentPower', 'acPowerMultiplier', 'acPowerDivisor']);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
    key:['temperature'],
        convertGet: async (entity, key, meta) => {
	        await entity.read('genDeviceTempCfg', ['currentTemperature']);
	    },
	    convertSet: async (entity, key, value, meta) => {
		    return null;
	    },
  },
};

let energy_divisor = 1;
let energy_multiplier = 1;
let voltage_divisor = 1;
let voltage_multiplier = 1;
let current_divisor = 1;
let current_multiplier = 1;
let power_divisor = 1;
let power_multiplier = 1;

const fzLocal = {
  tariff1: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('currentTier1SummDelivered')) {
            const data = msg.data['currentTier1SummDelivered'];
            result.Tariff1 = (parseInt(data[0]) << 32) + parseInt(data[1])/energy_divisor*energy_multiplier;
		}
        return result;
    },
  },
  tariff2: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('currentTier2SummDelivered')) {
            const data = msg.data['currentTier2SummDelivered'];
            result.Tariff2 = (parseInt(data[0]) << 32) + parseInt(data[1])/energy_divisor*energy_multiplier;
		}
        return result;
    },
  },
  tariff3: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('currentTier3SummDelivered')) {
            const data = msg.data['currentTier3SummDelivered'];
            result.Tariff3 = (parseInt(data[0]) << 32) + parseInt(data[1])/energy_divisor*energy_multiplier;
		}
        return result;
    },
  },
  tariff4: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('currentTier4SummDelivered')) {
            const data = msg.data['currentTier4SummDelivered'];
            result.Tariff4 = (parseInt(data[0]) << 32) + parseInt(data[1])/energy_divisor*energy_multiplier;
		}
        return result;
    },
  },
  serial_number: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('meterSerialNumber')) {
            const data = msg.data['meterSerialNumber'];
            result.SerialNumber = data.toString();
		}
        return result;
    },
  },
  date_release: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty(0xf003)) {
            const data = msg.data[0xf003];
            result.DateRelease = data.toString();
		}
        return result;
    },
  },
  model_name: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty(0xf004)) {
            const data = msg.data[0xf004];
            result.ModelName = data.toString();
		}
        return result;
    },
  },
  e_divisor: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('divisor')) {
          const data = parseInt(msg.data['divisor']);
          energy_divisor = data;
          result.e_divisor = energy_divisor;
		}
        return result;
    },
  },
  e_multiplier: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('multiplier')) {
          const data = parseInt(msg.data['multiplier']);
          energy_multiplier = data;
          result.e_multiplier = energy_multiplier;
		}
        return result;
    },
  },
  voltage: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('rmsVoltage')) {
          const data = parseInt(msg.data['rmsVoltage']);
          result.Voltage = data/voltage_divisor*voltage_multiplier;
          //meta.logger.info('Voltage: ' + data + ', multiplier: ' + voltage_multiplier + ', divisor: ' + voltage_divisor);
		}
        return result;
    },
  },
  v_multiplier: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('acVoltageMultiplier')) {
          const data = parseInt(msg.data['acVoltageMultiplier']);
          voltage_multiplier = data;
          result.v_multiplier = voltage_multiplier;
		}
        return result;
    },
  },
  v_divisor: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('acVoltageDivisor')) {
          const data = parseInt(msg.data['acVoltageDivisor']);
          voltage_divisor = data;
          result.v_divisor = voltage_divisor;
		}
        return result;
    },
  },
  current: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('instantaneousLineCurrent')) {
          const data = parseInt(msg.data['instantaneousLineCurrent']);
          result.Current = data/current_divisor*current_multiplier;
		}
        return result;
    },
  },
  c_multiplier: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('acCurrentMultiplier')) {
          const data = parseInt(msg.data['acCurrentMultiplier']);
          current_multiplier = data;
          result.c_multiplier = current_multiplier;
		}
        return result;
    },
  },
  c_divisor: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('acCurrentDivisor')) {
          const data = parseInt(msg.data['acCurrentDivisor']);
          current_divisor = data;
          result.c_divisor = current_divisor;
		}
        return result;
    },
  },
  power: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('apparentPower')) {
          const data = parseInt(msg.data['apparentPower']);
          result.Power = data/power_divisor*power_multiplier;
		}
        return result;
    },
  },
  p_multiplier: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('acPowerMultiplier')) {
          const data = parseInt(msg.data['acPowerMultiplier']);
          power_multiplier = data;
          result.p_multiplier = power_multiplier;
		}
        return result;
    },
  },
  p_divisor: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('acPowerDivisor')) {
          const data = parseInt(msg.data['acPowerDivisor']);
          power_divisor = data;
          result.p_divisor = power_divisor;
		}
        return result;
    },
  },
  temperature: {
    cluster: 'genDeviceTempCfg',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        if (msg.data.hasOwnProperty('currentTemperature')) {
          const data = parseInt(msg.data['currentTemperature']);
          result.Temperature = data;
		}
        return result;
    },
  },
  
}

const definition = {
    zigbeeModel: ['Electricity_meter_B85'], // The model ID from: Device with modelID 'lumi.sens' is not supported.
    model: 'Electricity Meter TLSR8258', // Vendor model number, look on the device for a model number
    vendor: 'DIY', // Vendor of the device (only used for documentation and startup logging)
    description: 'Electricity Meter', // Description of the device, copy from vendor site. (only used for documentation and startup logging)
    fromZigbee: [fzLocal.e_divisor, fzLocal.e_multiplier, fzLocal.tariff1, fzLocal.tariff2, fzLocal.tariff3, fzLocal.tariff4, 
                 fzLocal.serial_number, fzLocal.date_release, fzLocal.model_name,
                 fzLocal.v_multiplier, fzLocal.v_divisor, fzLocal.voltage,
                 fzLocal.c_multiplier, fzLocal.c_divisor, fzLocal.current,
                 fzLocal.p_multiplier, fzLocal.p_divisor, fzLocal.power,
                 fzLocal.temperature], // We will add this later
    toZigbee: [tzLocal.device_address_config, tzLocal.device_model_config, tzLocal.device_measurement_config, tzLocal.metering],
    meta: {
        multiEndpoint: true
    },
    configure: async (device, coordinatorEndpoint, logger) => {
      const firstEndpoint = device.getEndpoint(1);
      await reporting.bind(firstEndpoint, coordinatorEndpoint, ['seMetering', 'haElectricalMeasurement', 'genDeviceTempCfg']);
      const payload_e_multiplier = [{attribute: {ID: 769, type: 0x22}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_e_multiplier);
      const payload_e_divisor = [{attribute: {ID: 0x0302, type: 0x22}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_e_divisor);
      const payload_tariff1 = [{attribute: {ID: 0x0100, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff1);
      const payload_tariff2 = [{attribute: {ID: 0x0102, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff2);
      const payload_tariff3 = [{attribute: {ID: 0x0104, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff3);
      const payload_tariff4 = [{attribute: {ID: 0x0106, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff4);
      const payload_serial_number = [{attribute: {ID: 0x0308, type: 0x41}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_serial_number);
      const payload_date_release = [{attribute: {ID: 0xf003, type: 0x41}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_date_release);
      const payload_model_name = [{attribute: {ID: 0xf004, type: 0x41}, minimumReportInterval: 0, maximumReportInterval: 10, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_model_name);
      const payload_v_multiplier = [{attribute: {ID: 0x0600, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_v_multiplier);
      const payload_v_divisor = [{attribute: {ID: 0x0601, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_v_divisor);
      await reporting.rmsVoltage(firstEndpoint, {min: 0, max: 60, change: 0});
      const payload_c_multiplier = [{attribute: {ID: 0x0602, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_c_multiplier);
      const payload_c_divisor = [{attribute: {ID: 0x0603, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_c_divisor);
      const payload_current = [{attribute: {ID: 0x0501, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_current);
      const payload_p_multiplier = [{attribute: {ID: 0x0604, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_p_multiplier);
      const payload_p_divisor = [{attribute: {ID: 0x0605, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 60, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_p_divisor);
      await reporting.apparentPower(firstEndpoint, {min: 0, max: 60, change: 0});
      const payload_temperature = [{attribute: {ID: 0x0000, type: 0x29}, minimumReportInterval: 0, maximumReportInterval: 30, reportableChange: 0}];
      await firstEndpoint.configureReporting('genDeviceTempCfg', payload_temperature);
    },
// Should be empty, unless device can be controlled (e.g. lights, switches).
    exposes: [
      exposes.numeric('Tariff1', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 1'),
      exposes.numeric('Tariff2', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 2'),
      exposes.numeric('Tariff3', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 3'),
      exposes.numeric('Tariff4', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 4'),
      exposes.text('ModelName', ea.STATE_GET).withDescription('Meter Model Name'),
      exposes.text('SerialNumber', ea.STATE_GET).withDescription('Meter Serial Number'),
      exposes.text('DateRelease', ea.STATE_GET).withDescription('Meter Date Release'),
      exposes.numeric('Voltage', ea.STATE_GET).withUnit('V').withDescription('Voltage'),
      exposes.numeric('Current', ea.STATE_GET).withUnit('A').withDescription('Current'),
      exposes.numeric('Power', ea.STATE_GET).withUnit('kW').withDescription('Power'),
      exposes.numeric('Temperature', ea.STATE_GET).withUnit('°C').withDescription('Device temperature'),
      exposes.numeric('device_address_preset', ea.STATE_SET).withDescription('Device Address'),
      exposes.enum('device_model_preset', ea.STATE_SET, switchDeviceModel).withDescription('Device Model'),
      exposes.numeric('device_measurement_preset', ea.STATE_SET).withDescription('Measurement Period').withValueMin(1).withValueMax(255),
    ],
              
    ota: ota.zigbeeOTA,
};

module.exports = definition;


      // exposes.numeric('e_divisor', ea.STATE_GET).withDescription('Divisor of energy'),
      // exposes.numeric('e_multiplier', ea.STATE_GET).withDescription('Multiplier of energy'),
      // exposes.numeric('v_divisor', ea.STATE_GET).withDescription('Divisor of voltage'),
      // exposes.numeric('v_multiplier', ea.STATE_GET).withDescription('Multiplier of voltage'),
      // exposes.numeric('c_divisor', ea.STATE_GET).withDescription('Divisor of current'),
      // exposes.numeric('c_multiplier', ea.STATE_GET).withDescription('Multiplier of current'),
      // exposes.numeric('p_divisor', ea.STATE_GET).withDescription('Divisor of power'),
      // exposes.numeric('p_multiplier', ea.STATE_GET).withDescription('Multiplier of power'),
