#include "Servo.h"
#include "stm32f10x.h"                  // Device header



/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  * 修改说明：将PWM输出从PA1(TIM2_CH2)改为PA8(TIM1_CH1)
  */
void PWM_Init(void)
{
	/*开启时钟*/
	// 修改点1：将TIM2时钟改为TIM1时钟
	//RCC_APB2PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);			//开启TIM1的时钟 (TIM1在APB2总线上)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	// 修改点2：将引脚从PA1改为PA8
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;						
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA8引脚初始化为复用推挽输出	
																	//受外设控制的引脚，均需要配置为复用模式
	
	/*配置时钟源*/
	// 修改点3：将TIM2改为TIM1
	TIM_InternalClockConfig(TIM1);		//选择TIM1为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;				//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	// 修改点4：将TIM2改为TIM1
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM1的时基单元
	
	/*输出比较初始化*/ 
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值
	                                                                //则最好执行此函数，给结构体所有成员都赋一个默认值
	                                                                //避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	// 修改点5：将TIM2_CH2的初始化函数改为TIM1_CH1的初始化函数
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);                        //将结构体变量交给TIM_OC1Init，配置TIM1的输出比较通道1
	
	/*【关键修改点】TIM1主输出使能*/
	// TIM1是高级定时器，其PWM输出需要额外使能主输出
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	
	/*TIM使能*/
	// 修改点6：将TIM2改为TIM1
	TIM_Cmd(TIM1, ENABLE);			//使能TIM1，定时器开始运行
}

/**
  * 函    数：PWM设置CCR
  * 参    数：Compare 要写入的CCR的值，范围：0~19999 (对应ARR=20000-1)
  * 返 回 值：无
  * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
  *           占空比Duty = CCR / (ARR + 1)
  * 修改说明：函数名和内部调用都从通道2改为通道1，从TIM2改为TIM1
  */
void PWM_SetCompare2(uint16_t Compare)
{
	// 修改点7：将TIM2改为TIM1，SetCompare2改为SetCompare1
	TIM_SetCompare1(TIM1, Compare);		//设置CCR1的值
}


/**
  * 函    数：舵机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Servo_Init(void)
{
	PWM_Init();									//初始化舵机的底层PWM
}

/**
  * 函    数：舵机设置角度
  * 参    数：Angle 要设置的舵机角度，范围：0~180
  * 返 回 值：无
  */
void Servo_SetAngle(float Angle)
{
	PWM_SetCompare2(Angle / 180 * 2000 + 500);	//设置占空比
												//将角度线性变换，对应到舵机要求的占空比范围上
}

