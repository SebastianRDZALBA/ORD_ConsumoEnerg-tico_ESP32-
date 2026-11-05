
#include "sensor_ina.h"
#include "config.h"

#include <stdint.h>
#include <stddef.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#define REGISTRO_CONFIGURACION_INA219  0x00
#define REGISTRO_VOLTAJE_BUS_INA219    0x02
#define REGISTRO_POTENCIA_INA219       0x03
#define REGISTRO_CORRIENTE_INA219      0x04
#define REGISTRO_CALIBRACION_INA219    0x05

#define CONFIGURACION_INA219          0x3FFF // Promedio de 128 conversiones, lectura continua.

static i2c_master_bus_handle_t ManejadorBus = NULL;
static i2c_master_dev_handle_t ManejadorSensor = NULL;

static bool EscribirRegistroIna(uint8_t Registro, uint16_t Valor);
static bool LeerRegistroIna(uint8_t Registro, uint16_t *Valor);

static bool EscribirRegistroIna(uint8_t Registro, uint16_t Valor)
{
    if (ManejadorSensor == NULL)
    {
        return false;
    }

    uint8_t Datos[3] = {
        Registro,
        (uint8_t)(Valor >> 8),
        (uint8_t)(Valor & 0xFF)
    };

    if (i2c_master_transmit(
            ManejadorSensor,
            Datos,
            sizeof(Datos),
            TIEMPO_ESPERA_I2C_MS) != ESP_OK)
    {
        return false;
    }

    return true;
}

static bool LeerRegistroIna(uint8_t Registro, uint16_t *Valor)
{
    if (ManejadorSensor == NULL || Valor == NULL)
    {
        return false;
    }

    uint8_t DireccionRegistro = Registro;
    uint8_t Datos[2] = {0};

    if (i2c_master_transmit_receive(
            ManejadorSensor,
            &DireccionRegistro,
            1,
            Datos,
            sizeof(Datos),
            TIEMPO_ESPERA_I2C_MS) != ESP_OK)
    {
        return false;
    }

    *Valor = ((uint16_t)Datos[0] << 8) | Datos[1];

    return true;
}

bool InicializarSensorIna(void)
{
    if (ManejadorBus != NULL && ManejadorSensor != NULL)
    {
        return VerificarSensorIna();
    }

    i2c_master_bus_config_t ConfiguracionBus = {
        .i2c_port = PUERTO_I2C,
        .sda_io_num = PIN_SDA_INA219,
        .scl_io_num = PIN_SCL_INA219,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    if (i2c_new_master_bus(
            &ConfiguracionBus,
            &ManejadorBus) != ESP_OK)
    {
        ManejadorBus = NULL;
        return false;
    }

    i2c_device_config_t ConfiguracionSensor = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DIRECCION_INA219,
        .scl_speed_hz = FRECUENCIA_I2C_HZ
    };

    if (i2c_master_bus_add_device(
            ManejadorBus,
            &ConfiguracionSensor,
            &ManejadorSensor) != ESP_OK)
    {
        FinalizarSensorIna();
        return false;
    }

    if (!EscribirRegistroIna(
            REGISTRO_CONFIGURACION_INA219,
            CONFIGURACION_INA219))
    {
        FinalizarSensorIna();
        return false;
    }

    if (!EscribirRegistroIna(
            REGISTRO_CALIBRACION_INA219,
            CALIBRACION_INA219))
    {
        FinalizarSensorIna();
        return false;
    }

    if (!VerificarSensorIna())
    {
        FinalizarSensorIna();
        return false;
    }

    return true;
}

bool VerificarSensorIna(void)
{
    if (ManejadorSensor == NULL)
    {
        return false;
    }

    uint16_t ConfiguracionLeida = 0;
    uint16_t CalibracionLeida = 0;

    if (!LeerRegistroIna(
            REGISTRO_CONFIGURACION_INA219,
            &ConfiguracionLeida))
    {
        return false;
    }

    if (!LeerRegistroIna(
            REGISTRO_CALIBRACION_INA219,
            &CalibracionLeida))
    {
        return false;
    }

    if (ConfiguracionLeida != CONFIGURACION_INA219 ||
        CalibracionLeida != CALIBRACION_INA219)
    {
        return false;
    }

    return true;
}

bool LeerMedicionSensorIna(medicion_ina_t *Medicion)
{
    if (ManejadorSensor == NULL || Medicion == NULL)
    {
        return false;
    }

    uint16_t RegistroVoltaje = 0;
    uint16_t RegistroCorriente = 0;
    uint16_t RegistroCalibracion = 0;

    if (!LeerRegistroIna(
            REGISTRO_CALIBRACION_INA219,
            &RegistroCalibracion))
    {
        return false;
    }

    if (RegistroCalibracion != CALIBRACION_INA219)
    {
        return false;
    }

    if (!LeerRegistroIna(
            REGISTRO_VOLTAJE_BUS_INA219,
            &RegistroVoltaje))
    {
        return false;
    }

    if (!LeerRegistroIna(
            REGISTRO_CORRIENTE_INA219,
            &RegistroCorriente))
    {
        return false;
    }

    if ((RegistroVoltaje & 0x0001) != 0)
    {
        return false;
    }

    uint16_t ValorVoltaje = RegistroVoltaje >> 3;
    int16_t ValorCorriente = (int16_t)RegistroCorriente;

    medicion_ina_t NuevaMedicion = {
        .Voltaje_V = (ValorVoltaje * PASO_VOLTAJE_BUS_INA219_MV) / 1000.0f,
        .Corriente_mA = ValorCorriente * PASO_CORRIENTE_INA219_MA,
        .Potencia_mW = ((ValorVoltaje * PASO_VOLTAJE_BUS_INA219_MV) / 1000.0f) *
                        (ValorCorriente * PASO_CORRIENTE_INA219_MA)
    };

    if (NuevaMedicion.Voltaje_V < 1.0f ||
        NuevaMedicion.Voltaje_V > 26.0f ||
        NuevaMedicion.Corriente_mA < 0.0f ||
        NuevaMedicion.Corriente_mA > CORRIENTE_MAXIMA_INA219_MA)
    {
        return false;
    }

    *Medicion = NuevaMedicion;

    return true;
}

bool FinalizarSensorIna(void)
{
    if (ManejadorSensor != NULL)
    {
        if (i2c_master_bus_rm_device(ManejadorSensor) != ESP_OK)
        {
            return false;
        }

        ManejadorSensor = NULL;
    }

    if (ManejadorBus != NULL)
    {
        if (i2c_del_master_bus(ManejadorBus) != ESP_OK)
        {
            return false;
        }

        ManejadorBus = NULL;
    }

    return true;
}