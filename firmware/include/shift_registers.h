#ifndef __SHIFT_REGISTERS_H__
#define __SHIFT_REGISTERS_H__

#ifdef __cplusplus
extern "C"
{
#endif

void reset_cd22m_sr(void);
void reset_bruz_sr(void);
void set_bruz_sr_data(int data);
void set_cd22m_sr_data(int data);
void clock_bruz_sr(void);
void clock_cd22m_sr(void);

#ifdef __cplusplus
}
#endif

#endif /* __SHIFT_REGISTERS_H__ */
