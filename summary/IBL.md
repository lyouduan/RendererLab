# PBR与BRDF

## 是什么？

### **辐射度量学**

**1）立体角**

视为二维弧度扩展到三维，以球心望向球面的视野大小，即描述三维角度的度量值，单位立体弧度或者球弧度，Steradian(sr)，球面面积除以半径平方$\omega = s/r^2$.

**极坐标与立体角的转换: $\sin\theta d\theta d\phi = d \omega$**

**2）辐射通量**

**单位时间**的**辐射能量**:**$\Phi = \frac{dQ}{dt}$**

**3）辐射强度Radiant intensity**

**每单位立体角**的辐射通量大小: **$I = \frac{d\Phi}{d\omega}$**

**4）辐照度Irradiance**

照射表面的辐射通量，即**单位时间**内到达**单位面积**的**辐射通量**: $E= \frac{d\Phi}{dA}$

或者辐射率在摄入光所形成的半球上积分

$E = \int_\Omega L(\omega) \cos\theta d\omega$

辐射通量对于面积的密度

**5）辐射率Radiance**

**单位立体角**到达**单位投影面积**的**辐射通量**：$L=\frac{d^2\Phi}{dA \cos\theta d\omega}$

物体表面的微表面所接受来自某个方向光源的单位面积的光通量；



### **渲染方程与反射方程**

$L_o(p, \omega_o) =L_e + \int_\Omega f_r(p, \omega_i,\omega_o)L_i(p, \omega_i) \mathbf{n} \cdot \omega_i d \omega_i$

$L_o(p, \omega_o) = \int_\Omega f_r(p, \omega_i,\omega_o)L_i(p, \omega_i) \mathbf{n} \cdot \omega_i d \omega_i$

区别，渲染方程多一个**自发光项$L_e$.**

其中$f_r(p,\omega_i,\omega_o)$是BRDF；



### **BRDF是什么？**

描述光线经过某一表面如何反射到各个方向的分布函数

**定义**

出射辐射率的微分与入射辐照度的微分之比：

$f_r(l,v) = \frac{d L_o(v)}{d E(l)}$

**某一表面从入射方向l接收到的辐照度会反射到各个方向上，其中给定一个出射方向v时，则BRDF决定了该方向v上会接收到多少反射光。**



#### **漫反射BRDF**

漫反射即某一表面各个方向均匀反射，即$f_r$不取决于任何方向

$L_o(p, \omega_o) = f_r\int_\Omega L_i(p, \omega_i) \mathbf{n} \cdot \omega_i d \omega_i$

将$f_r$从积分中提出来，再假定满足能量守恒定律，即入射光全部反射，没有任何损耗$L_o = L_i$

$L_o(p, \omega_o) = f_r\int_\Omega L_i(p, \omega_i) \mathbf{n} \cdot \omega_i d \omega_i$

$f_r\int_\Omega \mathbf{n} \cdot \omega_i d \omega_i = 1$

$f_r\int_0^{2\pi} \int_0^{\pi/2} \cos\theta sin\theta d\theta d\phi = 1$

$f_r 2\pi \int_0^{\pi/2} \cos\theta sin\theta d\theta = 1$

$f_r = \frac{1}{\pi}$



#### **镜面反射BRDF**

常见模型有Cook-Torrance

$f_r = \frac{D(h)F(v,l)G(l,v,h)}{4(n,v)(n,l)}$

其中D为法线分布函数，F为菲涅尔项，G为几何函数；

##### **法线分布函数DNF**

微表面模型下，每个微平面都理想为镜面反射，其法线为**m**，其中正好反射到v方向的法线$h = \frac{v+l}{|v + l|}$。

本质上，DNF统计微平面法线**m**，有多少法线**m=h**的情况，

##### **菲涅尔项Fresnel**

描述物体表面反射强度与视角的关系，视角由法线与出射方向的夹角定义，夹角越大，反射率越大；

一般使用Schlick 近似

$F = F_0 + (1 - F_0)(1-\cos\theta)^5$

其中$F_0$为垂直视角下，物体表面的基础反射率

##### **几何函数**

微表面模型下，不同微平面之间存在相互遮挡，光线被遮挡或者视线被遮挡，即自阴影和自遮挡，只有光线与视线同时可见，微平面才可见。

##### **法线分布校正因子$4(n,v)(n,l)$**

如何计算？

BRDF定义式$f_r(l,v) = \frac{d L_o(v)}{d E(l)} = \frac{d L(w_o)}{L(\omega_i) \cos\theta_i d\omega_i}$

计算$d L_o(v)$

首先需要知道入射的辐射通量，只有法线m=h时，才对出射方向有贡献；

所以只需统计法线为h时的微平面，利用法线分布D(h);

$d^2 A(w_h) = D(w_h)dw_h dA $

$d^2 \Phi = L_i(w_i)dw_i d^2 A^{\perp}(w_h) $

$=L_i(\omega_i)d\omega_i \cos\theta d^2 A(w_h) $

=$L_i(\omega_i)d\omega_i \cos\theta D(w_h)dw_h dA$



$dL_o = \frac{d^3 \Phi_o}{d\omega_o\cos\theta_o dA }$

$=\frac{L_i(\omega_i)d\omega_i \cos\theta_h D(w_h)dw_h dA}{d\omega_o\cos\theta_o dA }$

$=\frac{L_i(\omega_i)d\omega_i \cos\theta_h D(w_h)dw_h}{d\omega_o\cos\theta_o }$



$f_r(l,v) = \frac{d L_o(v)}{d E(l)} = \frac{d L(w_o)}{L(\omega_i) \cos\theta_i d\omega_i}$

$=\frac{\frac{L(\omega_i)d\omega_i \cos\theta_h D(w_h)dw_h}{d\omega_o\cos\theta_o }}{L(\omega_i) \cos\theta_i d\omega_i}$

$=\frac{\cos\theta_h D(w_h)dw_h}{\cos\theta_i \cos\theta_o d\omega_o}$

**未知量$\frac{d w_h}{ d w_o}$**

$d_w = \sin\theta d\theta d\phi$

$\frac{d w_h}{ d w_o} = \frac{\sin\theta_h d\theta_h d\phi}{\sin\theta_o d\theta_o d\phi} = \frac{\sin\theta_h d\theta_h}{\sin\theta_o d\theta_o}$

由镜面反射可知，$\theta_o = 2\theta_h$

$\frac{d w_h}{ d w_o} = \frac{\sin\theta_h d\theta_h}{\sin 2\theta_h d2\theta_h} = \frac{\sin\theta_h d\theta_h}{2* 2\sin \theta_h \cos \theta_h d\theta_h} $

$= \frac{1}{4\cos \theta_h }$

带入$f_r =\frac{\cos\theta_h D(w_h)dw_h}{\cos\theta_i \cos\theta_o d\omega_o} $

$=\frac{\cos\theta_h D(w_h)}{\cos\theta_i \cos\theta_o 4\cos\theta_h} =\frac{D(w_h)}{4\cos\theta_i \cos\theta_o } $

不难看出，是对法线分布的校正因子；

**同时，法线分布函数应该满足，所有微平面的投影面积等于宏观投影面积**



### 微表面模型Cook-Torrance

**Cook-Torrance BRDF**

$f_r = k_d f_{lambert} + k_s f_{cook-torrance}$

$k_s+k_d = 1$，$k_s$为菲涅尔项，$f_{lambert}$为漫反射BRDF，$f_{cook-torrance}$为镜面反射BRDF。

由于$f_{cook-torrance}$已经包含了菲涅尔项，应该不再加$k_s$。

即最终的**Cook-Torrance BRDF**为

**$f_r = (1-F) f_{lambert} + f_{cook-torrance}$**



### **金属度Metallic**

**在BRDF中加入metallic能够有效将各种材质（金属和非金属）使用albedo和metallic整合到一张texture中，再通过插值，计算出F0**

$F_o = lerp(0.04, albedo, metallic)$

**$f_r = (1-F)(1-metallic) f_{lambert} + f_{cook-torrance}$**



**IBL**

2.怎样做？

3.为什么？





# 基于图像的照明IBL

## 是什么？

基于物理的环境光照明技术，把**环境贴图Cubemap**作为**环境光**，增加场景真实性。

IBL通过对Cubemap采样，模拟环境光，实现环境光的漫反射和镜面反射。

其中，基于物理的渲染BPR涉及BRDF模型，描述入射光经过某一表面将光能反射到各个方向的分布

**即解决反射方程从各个方向的环境光**



## **反射方程**

$L_o(p, \omega_o) = \int_\Omega f_r(p, \omega_i, \omega_o) L_i(p, \omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

BRDF常用Cook-Torrance模型，由于BRDF的线性特征，因此可将反射方程拆分为漫反射+镜面反射两个部分（省略着色点p）：

$f_r = k_d f_d+ f_s$

$L_o(\omega_o) = \int_\Omega k_d f_d(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i $

+$ \int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

**使用蒙特卡洛方法采样，在实时渲染中不可行，利用预计算进行解决**



## **求解漫反射$L_d$**

$L_d = \int_\Omega k_d f_d(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

由于漫反射BRDF:$f_d = \frac{1}{\pi}$为常数，提出积分外，

$L_d = f_d\int_\Omega  k_d L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

$k_d = (1-F(w_i\cdot \mathbf{n}))(1-metallic)$

需要进一步简化，将$k_d$移除积分式子：

$k_d \approx (1 - F_{roughness}(\mathbf{n} \cdot w_o))(1 -metallic) \triangleq k_d^*$

$F_{roughness}(\mathbf{n} \cdot w_o) = F_0 + (\Lambda - F_0)(1 - n \cdot \omega_o)^5)$

$\Lambda = \max\{1-roughness, F_0\}$

**最终简化为**

$L_d =  f_d k_d^* \ \int_\Omega  L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

此时$L_d$积分与法线$\mathbf{n}$所在半球面有关，可预计算出Irradiance Map，使用法线$\mathbf{n}$采样，即可得到对应的漫反射辐射率$L_d$。



**均匀分布蒙特卡洛方法**

由于漫反射均匀反射光线，所以只需要使用蒙特卡洛方法对Environment Map进行均匀积分，**将$f_d$也放入预计算中，减少除法计算**。

$L_d = \frac{1}{\pi} \int_\Omega  L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

$= = \frac{1}{\pi} \int_0^{2\pi}\int_0^{\frac{\pi}{2}} L_i(p,\theta_i,\phi) \cos\theta_i \sin\theta_i d \theta_i d \phi$

$\approx \frac{1}{\pi}  \frac{1}{N_1 N_2} \sum_i^N \sum_j^N \frac{L_i(p, \theta_i, \phi_j) \cos\theta_i \sin\theta_i}{p(\theta_i,\phi_j)}$

$\approx \frac{1}{\pi}  \frac{1}{N_1 N_2} \sum_i^N \sum_j^N \frac{L_i(p, \theta_i, \phi_j) \cos\theta_i \sin\theta_i}{\frac{1}{2*\pi \cdot 0.5 * \pi}}$

$\approx \pi  \frac{1}{N_1 N_2} \sum_i^N \sum_j^NL_i(p, \theta_i, \phi_j) \cos\theta_i \sin\theta_i$

$p(\theta,\phi)$为积分区域的均匀采样的均匀分布概率；



**余弦加权采样**

由于光照遵循lambert余弦定律，即入射光线与法线夹角越大，单位立体角投影到单位面积的**辐射强度Radiant Intensity**越小：

$L_\theta = \frac{I_\theta}{dA\cos\theta}$

由于Radiance沿着立体角传播时保持不变，因此，即$\theta$越大，$\cos\theta$越小，相应的$I_\theta应该变小$

$立体角d\omega_i$与$\cos\theta_i$正相关，所以

半球的立体角为$\int_\Omega \mathbf{n} \cdot \omega_i d\omega_i = \pi$

因此概率为$p(w_i)=\frac{\mathbf{n} \cdot \omega_i }{\pi}$

$L_d = \frac{1}{\pi} \int_\Omega  L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

$=\frac{1} {\pi} \frac{1}{N}\sum_i^N \frac{L_i(\omega_i) \mathbf{n} \cdot \omega_i}{p(w_i)} $

$=\frac{1}{\pi} \frac{1}{N}\sum_i^N \frac{L_i(\omega_i) \mathbf{n} \cdot \omega_i}{\frac{\mathbf{n} \cdot \omega_i }{\pi}} $

$=\frac{1}{N} \sum_i^N L_i(\omega_i) $



## 求解镜面反射$L_S$

$L_s = \int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

镜面反射BRDF：$f_s(\omega_i, \omega_o)= \frac{F(v,l)D(h)G(v,l,h)}{4(n\cdot l)(n\cdot v)}$

涉及入射方向、出射方向，粗糙度等多个变量，无法直接拆分解决；



### **Spilt Sum Approximation**

$L_s = \int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

$\approx  \frac{\int_{\Omega_{fr}} L_i(w_i)dw_i}{\int_{\Omega_{fr}} dw_i}  *\int_\Omega f_s(\omega_i, \omega_o) \mathbf{n} \cdot \omega_i d\omega_i$

$\approx (\frac{\sum_k^N L_i(\omega_i^{(k)})W(\omega_i^{(k)})}{\sum_k^N W(\omega_i^{(k)})}) (\frac{1}{N} \sum_j^N \frac{f_s \mathbf{n} \cdot \omega_i^{(j)}}{p(\omega_i^{(k)})})$



**拆分和近似**将$L_s$拆分成两个部分分别采样处理；

#### **Prefilter Map**

**推理前半部分**

首先把$L_i$拆分出来，

$L_s=\int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

$= \frac{\int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i}{\int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i} \int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i$

$= L_c(w_o) \ \int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i$

**蒙特卡洛重要性采样**

>对法线分布函数D(h)进行重要性采样，这是由于并不是所有微平面会对$L_s$有贡献，只有**微平面法线m=h**时才有贡献。
>
>归一化D(h)满足$\int \frac{D(w_h) (w_h \cdot \mathbf{n}) }{4 (w_h \cdot w_o)}dw_h = 1$
>
>$\int D(w_h) (w_h \cdot \mathbf{n}) dw_h = 1$
>
>$p(w_i) = \frac{D(w_h) (w_h \cdot \mathbf{n}) }{4 (w_o \cdot w_h \cdot w_o)}$

$\frac{\int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i}{\int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i}$

**对分子采样，给定入射方向，当出射方向确定时，半程向量h也确定，只需要对出射方向$w_i$或者$w_h$任一采样即可**

$\int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i$

$\approx \frac{1}{N} \sum_k^N\frac{D(w_h^{(k)}) F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)}) (\mathbf{n} \cdot w_i^{(k)})}{4(w_i^{(k)} \cdot \mathbf{n})(w_o \cdot \mathbf{n}) \ p(w_i^{(k)})}$

$\approx \frac{1}{N} \sum_k^N\frac{D(w_h^{(k)}) F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)}) (\mathbf{n} \cdot w_i^{(k)})}{4(w_i^{(k)} \cdot \mathbf{n})(w_o \cdot \mathbf{n}) \  \frac{D(w_h^{(k)}) (w_h^{(k)} \cdot \mathbf{n}) }{4 (w_h^{(k)} \cdot w_o)}}$

$\approx \frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)})}{(w_o \cdot \mathbf{n}) \  \frac{(w_h \cdot \mathbf{n}) }{ (w_h \cdot w_o)}}$

$\approx \frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)}) (w_h \cdot w_o)}{(w_o \cdot \mathbf{n}) \ (w_h \cdot \mathbf{n})}$

**分母同理**

$\int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i$

$\approx \frac{1}{N} \sum_k^N\frac{D(w_h^{(k)}) F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) (\mathbf{n} \cdot w_i^{(k)})}{4(w_i^{(k)} \cdot \mathbf{n})(w_o \cdot \mathbf{n}) \  \frac{D(w_h^{(k)}) (w_h^{(k)} \cdot \mathbf{n}) }{4 (w_h^{(k)} \cdot w_o)}}$

$\approx \frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h)}{(w_o \cdot \mathbf{n}) \  \frac{(w_h \cdot \mathbf{n}) }{ (w_h \cdot w_o)}}$

$\approx \frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h)(w_h \cdot w_o)}{(w_o \cdot \mathbf{n}) \ (w_h \cdot \mathbf{n})}$



$\frac{\int_\Omega f_s(\omega_i, \omega_o) L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i}{\int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i}$

$\approx \frac{\frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)}) (w_h \cdot w_o)}{(w_o \cdot \mathbf{n}) \ (w_h \cdot \mathbf{n})}}{\frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h)(w_h \cdot w_o)}{(w_o \cdot \mathbf{n}) \ (w_h \cdot \mathbf{n})}}$

$\approx \frac{\frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)})  (w_h^{(k)} \cdot w_o)}{(w_h^{(k)} \cdot \mathbf{n})}}{\frac{1}{N} \sum_k^N\frac{F(w_i^{(k)}, w_o) G(w_i^{(k)},w_o, w_h)(w_h^{(k)} \cdot w_o)}{(w_h^{(k)} \cdot \mathbf{n})}}$

其中，F对结果的影响不大：

1）对于光滑的情况，$w_h$接近n，F接近于定值；

2）对于非光滑的情况，L本身比较粗略近似，F对结果影响不大。

因此约掉F项

$L_c(w_o) \approx \frac{\sum_k^N\frac{ G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)}) (w_h^{(k)} \cdot w_o)}{(w_h^{(k)} \cdot \mathbf{n})}}{\sum_k^N\frac{ G(w_i^{(k)},w_o, w_h)(w_h \cdot w_o)}{(w_h^{(k)} \cdot \mathbf{n})}}$

>看回原来的式子$L_c(w_o)=  \frac{\int_{\Omega_{fr}} L_i(w_i)dw_i}{\int_{\Omega_{fr}} dw_i} $
>
>**式子是对BRDF反射的区域进行积分，本质上就是对Environment Map进行卷积操作。**
>
>**卷积核大小取决于BRDF的反射方向R的反射波瓣（lobe）大小，反射波瓣大小取决于粗糙度roughness。**
>
>**通过卷积后，得到prefilterMap，使用反射方向R进行查询取值。**

由于镜面反射BRDF**反射方向主要聚集在反射波瓣附近**$R=reflect(-w_o, \mathbf{n})$

**因此，进一步近似简化采样，假设$w_o = n = R$，**$f(w_o, w_i(n),n) = f(R, w_i(R),R)$

$L_c(R) \approx \frac{\sum_k^N\frac{ G(w_i^{(k)},w_o, w_h) L_i(p,w_i^{(k)}) (w_h^{(k)} \cdot w_o)}{(w_h^{(k)} \cdot \mathbf{n})}}{\sum_k^N\frac{ G(w_i^{(k)},w_o, w_h)(w_h \cdot w_o)}{(w_h^{(k)} \cdot \mathbf{n})}}$

$= \frac{\sum_k^N\frac{ G(w_i^{(k)},R, R) L_i(p,w_i^{(k)}) (R \cdot w_o)}{(R \cdot \mathbf{n})}}{\sum_k^N\frac{ G(w_i^{(k)},R, R)(R\cdot w_o)}{(R\cdot \mathbf{n})}}$

$= \frac{ \sum_k^N G(w_i^{(k)},R, R) L_i(p,w_i^{(k)})}{\sum_k^NG(w_i^{(k)},R, R)}$

$= \frac{ \sum_k^N G(w_i^{(k)},R, R) L_i(p,w_i^{(k)})}{\sum_k^NG(w_i^{(k)},R, R)}$

$= \frac{ \sum_k^N G(w_i^{(k)},R, R) L_i(p,w_i^{(k)})}{\sum_k^NG(w_i^{(k)},R, R)}$

此时$L_c(R)$剩下两个变量，粗糙度和反射方向R，通过mipmap方式设定粗糙度为定值，再通过插值进行计算，所以只有一个变量R，即可以获得prefilterMap。

**误差**

而由于设定$w_o = n = R$，忽略了出射角和法线的具体方向，因此在掠射角的表面上，无法获得拖长的反射现象；



#### BRDF LUT

$\int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i$

积分中包含BRDF项（粗糙度，法线与入射方向，基础反射率$F_0$），其中法线与入射方向的夹角$\cos\theta$

总共三个变量：粗糙度，基础反射率$F_0$，$\cos\theta$;

首先，把$F_0$从积分中拆分出来，

$\int_\Omega f_s(\omega_i, \omega_o)  \mathbf{n} \cdot \omega_i d\omega_i$

$=\int_\Omega \frac{f_s}{F} \ F \ \mathbf{n} \cdot \omega_i d\omega_i$

$=\int_\Omega \frac{f_s}{F} \ ( F_0+ (1-F_0)(1-(\mathbf{n} \cdot \omega_i))^5) \ (\mathbf{n} \cdot \omega_i )d\omega_i$

$=\int_\Omega \frac{f_s}{F} \ ( F_0-F_0(1-(\mathbf{n} \cdot \omega_i))^5 + (1-(\mathbf{n} \cdot \omega_i))^5) \ (\mathbf{n} \cdot \omega_i )d\omega_i$

$=\int_\Omega \frac{f_s}{F} \ (F_0( 1-(1-(\mathbf{n} \cdot \omega_i))^5 + (1-(\mathbf{n} \cdot \omega_i))^5) \ (\mathbf{n} \cdot \omega_i )d\omega_i$

$=F_0 \int_\Omega \frac{f_s}{F} \ ( 1-(1-(\mathbf{n} \cdot \omega_i))^5 (\mathbf{n} \cdot \omega_i )d\omega_i +\int_\Omega \frac{f_s}{F} (1-(\mathbf{n} \cdot \omega_i))^5 \ (\mathbf{n} \cdot \omega_i )d\omega_i$

由于$f_s$中包含F，所以可以约掉

$F_0 \int_\Omega DG \ ( 1-(1-(\mathbf{n} \cdot \omega_i))^5 (\mathbf{n} \cdot \omega_i )d\omega_i +\int_\Omega DG (1-(\mathbf{n} \cdot \omega_i))^5 \ (\mathbf{n} \cdot \omega_i )d\omega_i$

上式子积分中包含粗糙度和$\cos\theta$两个变量

分别对$\int_\Omega DG \ ( 1-(1-(\mathbf{n} \cdot \omega_i))^5 (\mathbf{n} \cdot \omega_i )d\omega_i$和$\int_\Omega DG(1-(\mathbf{n} \cdot \omega_i))^5 \ (\mathbf{n} \cdot \omega_i )d\omega_i$根据法线分布函数进行重要性采样蒙特卡洛积分；

$\int_\Omega DG \ ( 1-(1-(\mathbf{n} \cdot \omega_i))^5 (\mathbf{n} \cdot \omega_i )d\omega_i$

$\approx \frac{1}{N} \sum_k^N \frac{D(w_h^{(k)}) G(w_o, w_i^{(k)},w_h) \ (1-(1-\mathbf{n} \cdot w_i^{(k)}))^5 (\mathbf{n} \cdot \omega_i^{(k)} )}{4(w_o \cdot n)(w_i^{(k)} \cdot n) p(w_i^{(k)})} $

$= \frac{1}{N} \sum_k^N \frac{D(w_h^{(k)}) G(w_o, w_i^{(k)},w_h) \ (1-(1-\mathbf{n} \cdot w_i^{(k)}))^5 (\mathbf{n} \cdot \omega_i^{(k)} )}{4(w_o \cdot n)(w_i^{(k)} \cdot n) \frac{D(w_h^{(k)}) (w_h^{(k)} \cdot \mathbf{n}) }{4 (w_h^{(k)} \cdot w_o)}} $

$= \frac{1}{N} \sum_k^N \frac{G(w_o, w_i^{(k)},w_h) \ (1-(1-\mathbf{n} \cdot w_i^{(k)}))^5 (w_h^{(k)} \cdot w_o)}{(w_o \cdot n)(w_i^{(k)} \cdot n) } $

$\int_\Omega DG \ ( (1-(\mathbf{n} \cdot \omega_i))^5 (\mathbf{n} \cdot \omega_i )d\omega_i$同理

两个部分的积分值都是1D变量，可以存放在同一2D纹理的RG两个通道中，即Lookup Texture(LUT)。

LUT由BRDF决定，BRDF给定，LUT也同样确定



## **IBL实现**

通过预计算的方式，获得了Irradiance Map、Prefilter Map和BRDF LUT三张纹理贴图；

最终近似结果：

$L_o = k_d^* \rho \ IrradianceMap(\mathbf{n})$

 $+ PrefilterMap(roughness*Mipmaps,R) * LUT(roughness, \cos\theta)$



