# RendererLab
Experiments on advanced rendering techniques



# IBL





基于图像照明，用于计算PBR的环境光照

**环境光照：漫反射+镜面反射**

**PBR：BRDF、Fresnel、energy conserving、 Microfacet theory**

Cubemap

**微平面模型Cook-Torrance**
$$
f_r(N, V, L) = k_d f_d + f_s \\
f_d = \frac{\rho}{\pi} \\
k_s = F\\
f_s = k_s \frac{DG}{4(N \cdot V)(N \ cdot L)} =\frac{FDG}{4(N \cdot V)(N \ cdot L)}\\
k_d + k_s = 1
$$
$k_d$漫反射系数，$k_s$镜面反射系数，$k_d + k_s = 1$保证能量守恒



## 漫反射与Irradiance Map

**Lambert's Diffuse**
$$
\large L_d(\omega_o) & = \int_\Omega k_d f_d L_i(p, \omega_i) \, \mathbf{n} \cdot \omega_i \, d\omega_i   \\ 
&= \int_{\Omega} \frac{\rho}{\pi} k_d L_i(p, \omega_i) \, \mathbf{n} \cdot \omega_i \, d\omega_{i}   \\&
$$
其中，$k_d = (1 - F(\mathbf{n} \cdot \omega_i))(1- metalness)$，并不能直接将$k_d$直接移出积分式，其跟$\mathbf{n}和\omega_i$相关。

在lambertian BRDF中， $f_d$是常数，$f_d=\frac{1}{\pi}$；主要需要处理$k_d$中的**$ F(\mathbf{n} \cdot \omega_i)$**.

**简化$k_d$**
$$
k_d \approx (1 - F_{\text{roughness}}(n \cdot \omega_o))(1 - \text{metalness}) \triangleq k_d^* \\
F_{\text{roughness}}(n \cdot \omega_o) = F_0 + (\Lambda - F_0)(1 - n \cdot \omega_o)^5\\
\Lambda = \max\{1 - \text{roughness}, F_0\}
$$
**朗伯体漫反射项**
$$
L_d =& \frac{\rho}{\pi} \int_\Omega k_d L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i \\
\approx&k_d^* \frac{\rho}{\pi} \int_\Omega L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i
$$
积分式子（5）只取决于$\mathbf{n}$，对以$\mathbf{n}$为半球的区域进行积分。



**积分方法：蒙特卡洛积分**

蒙特卡洛基于大数定律，采样样本越多，结果越准确。

**均匀分布蒙特卡洛积分**
$$
\frac{1}{\pi} \int_\Omega L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i
&= \frac{1}{\pi} \int_0^{2\pi} \int_0^{\frac{\pi}{2}} L_i (p, \omega_i) \mathbf{n} \cos\theta \sin\theta  d\theta d\phi\\
&= \frac{1}{\pi} \frac{1}{N_1N_2} \sum_i^{N_1}\sum_j^{N_2} \frac{L_i(p, \phi_j, \theta_i) \cos\theta_i \sin\theta_i}{p(\theta_i, \phi_i)} \\
&=\frac{1}{\pi} \frac{1}{N_1N_2} \sum_i^{N_1}\sum_j^{N_2} \frac{L_i(p, \phi_j, \theta_i) \cos\theta_i \sin\theta_i}{\frac{1}{2\pi * 0.5\pi}}\\
& = \pi \frac{1}{N_1N_2} \sum_i^{N_1}\sum_j^{N_2}L_i(p, \phi_j, \theta_i) \cos\theta_i \sin\theta_i
$$
$p(\theta, \phi)$为概率密度函数，$\theta \in[0, 2\pi],\phi \in [0, 0.5\pi]$，相互独立，因此为$p = \frac{1}{2\pi * 0.5\pi}$

或者以余弦为权重作为采样

**Cosine-Weighted Hemisphere Sampling**

这是因为光照模型符合lambertian reflection, 以余弦分布的方式照射在表面上
$$
L_\theta = \frac{I_\theta}{dA\cos\theta}
$$
不同角度的辐射强度随着角度增大而减弱，垂直照射时，单位面积的辐射强度最强。

**因此，在采样时，对光照进行重要性采样，入射光线与法线方向角度越小，光照强度越大，越可能被采样**
$$
L_d =& \frac{1}{\pi} \int_\Omega L_i(\omega_i) \mathbf{n} \cdot \omega_i d\omega_i \\
\approx& \frac{1}{\pi}\frac{1}{N} \sum_i^N \frac{L(\omega_i)(\mathbf{n}{\omega_i})}{\frac{\mathbf{n} \cdot {\omega_i}}{\pi}}
=& \frac{1}{N} \sum_i^N L(\omega_i)
$$
**注意，计算时把$\frac{1}{\pi}$也加入到irradiance map中，减少在shader计算时的除$\pi$操作**



预计算得到Irradiance Map后，对其采样结果乘以$k_d \rho$即可以得到**漫反射项**

## 镜面反射

$$
\large L_s(\omega_o)  = \int_\Omega f_s L_i(p, \omega_i) \, \mathbf{n} \cdot \omega_i \, d\omega_i \\
f_s = k_s \frac{DG}{4(N \cdot V)(N \ cdot L)} =\frac{FDG}{4(N \cdot V)(N \ cdot L)}\\
$$

$L_s$取决于$\mathbf{n}和\omega_i$，以及$f_s$中包含roughness和$F_0$，无法直接解决。



### **spilt sum approximation**

$$
\large L_s(\omega_o)  &= \int_\Omega f_s L_i(p, \omega_i) \, \mathbf{n} \cdot \omega_i \, d\omega_i \\
&\approx \frac{\int_{\Omega_{f_r}} L_i(p, \omega_i) d\omega_i}{\int_{\Omega_{f_r}} d \omega_i} \ * \ \int_\Omega f_s \mathbf{n} \cdot \omega_i d \omega_i  \\
&\approx (\frac{\sum_k^N L_i(\omega_i^{(k)})W(\omega_i^{(k)})}{\sum_k^N W(\omega_i^{(k)})}) (\frac{1}{N} \sum_j^N \frac{f_s \mathbf{n} \cdot \omega_i^{(j)}}{p(\omega_i^{(k)})})
$$

**第一步,将$L_i$从积分式子中拆出来：prefilter map**
$$
\large L_s(\omega_o)  &= L_c(\omega_o)\int_\Omega f_s  \, \mathbf{n} \cdot \omega_i \, d\omega_i \\
L_c(\omega_o) &=\frac{\int_\Omega f_s L_i(p, \omega_i) \, \mathbf{n} \cdot \omega_i \, d\omega_i \\}{\int_\Omega f_s \, \mathbf{n} \cdot \omega_i \, d\omega_i \\}
$$
**使用法线分布函数NDF进行重要性采样**
$$
\large \int D(\omega_h) (\omega_h \cdot \mathbf{n}) d\omega_h = \int \frac{D(\omega_h) (\omega_h \cdot \mathbf{n})}{4(\omega_h \cdot \omega_o)} d\omega_i  = 1\\
\large thus, \ \ \ p(\omega_i) = \frac{D(\omega_h)(\omega_h \cdot \mathbf{n})}{4(\omega_h \cdot \omega_o)}
$$

$$
\large L_c \approx \frac{\frac{1}{N} \sum_k^N   \frac{D(\omega_h^{(k)}) \, F(\omega_o \cdot \omega_i^{(k)}) G(\omega_o, \omega_i^{(k)}) L_i(p, \omega_i) (\mathbf{n} \cdot \omega_i^{(k)})}{4 (\omega_o \cdot \mathbf{n})(\omega_i^{(k)}, \mathbf{n}) \ p(w_i^{(k)})}}{\frac{1}{N} \sum_k^N \frac{ D(\omega_h^{(k)}) \, F(\omega_o \cdot \omega_i^{(k)})G(\omega_o, \omega_i^{(k)}) (\mathbf{n} \cdot \omega_i^{(k)})}{4 (\omega_o \cdot \mathbf{n})(\omega_i^{(k)}, \mathbf{n}) \ p(w_i^{(k)})}} \\

\large  =\frac{\sum_k^N \frac{D(\omega_h^{(k)}) \, F(\omega_o \cdot \omega_i^{(k)}) G(\omega_o, \omega_i^{(k)}) L_i(p, \omega_i) (\mathbf{n} \cdot \omega_i^{(k)})}{4 (\omega_o \cdot \mathbf{n})(\omega_i^{(k)}, \mathbf{n}) \  \frac{D(\omega_h)(\omega_h \cdot \mathbf{n})}{4(\omega_h \cdot \omega_o)}}}{\sum_k^N  \frac{D(\omega_h^{(k)}) \, F(\omega_o \cdot \omega_i^{(k)})G(\omega_o, \omega_i^{(k)}) (\mathbf{n} \cdot \omega_i^{(k)})}{4 (\omega_o \cdot \mathbf{n})(\omega_i^{(k)}, \mathbf{n}) \  \frac{D(\omega_h)(\omega_h \cdot \mathbf{n})}{4(\omega_h \cdot \omega_o)}}} \\

\large  =\frac{\sum_k^N \frac{F(\omega_o \cdot \omega_i^{(k)}) G(\omega_o, \omega_i^{(k)}) L_i(p, \omega_i) }{\frac{(\omega_h \cdot \mathbf{n})}{(\omega_h \cdot \omega_o)}}}{\sum_k^N \frac{F(\omega_o \cdot \omega_i^{(k)}) G(\omega_o, \omega_i^{(k)}) }{\frac{(\omega_h \cdot \mathbf{n})}{(\omega_h \cdot \omega_o)}}} \\

\large  =\frac{\sum_k^N \frac{F(\omega_o \cdot \omega_i^{(k)}) G(\omega_o, \omega_i^{(k)}) L_i(p, \omega_i) (\omega_h^{(k)} \cdot \omega_o) }{\omega_h^{(k)} \cdot \mathbf{n}}}{\sum_k^N \frac{F(\omega_o \cdot \omega_i^{(k)}) G(\omega_o, \omega_i^{(k)}) (\omega_h^{(k)} \cdot \omega_o)}{\omega_h^{(k)} \cdot \mathbf{n}}} \\
$$

由于F影响不大，进一步去掉F：

1）对于光滑的情况，$w_h$接近$\mathbf{n}$，所以F基本为定值，即F0，参考Fresnel曲线；

2）对于非光滑的情况，$L_c$变得非常模糊，去掉F，对结果已经影响不大。

![image-Fresnel](.\pic\fresnel.png)

![image-F](.\pic\Refectance.png)

**去掉F项后**
$$
\large  L_c=\frac{\sum_k^N \frac{G(\omega_o, \omega_i^{(k)}) L_i(p, \omega_i) (\omega_h^{(k)} \cdot \omega_o) }{\omega_h^{(k)} \cdot \mathbf{n}}}{\sum_k^N \frac{G(\omega_o, \omega_i^{(k)}) (\omega_h^{(k)} \cdot \omega_o)}{\omega_h^{(k)} \cdot \mathbf{n}}} \\
$$
由于镜面反射的BRDF的**反射波瓣取决于反射反向$R=reflect(-\omega_o, \mathbf{n})$**，同时**波瓣大小的取决于粗糙程度**。

因此，不同的入射方向对于波瓣的形状变化不大，**由于出射方向$\omega_o$未知，进一步做了近似处理**：

$f_s(\omega_o, \omega_i(\mathbf{n}), \mathbf{n}) = f(\mathbf{R},\omega_i(\mathbf{R}), \mathbf{R})$

**即令V=N=R，法线方向，入射方向，反射方向一致**
$$
\large  L_c(R) \approx \frac{\sum_k^N \frac{G(\mathbf{R}, \omega_i^{(k)}) L_i(p, \omega_i) (\omega_h^{(k)} \cdot \mathbf{R}) }{\omega_h^{(k)} \cdot \mathbf{n}}}{\sum_k^N \frac{G(\mathbf{R}, \omega_i^{(k)}) (\omega_h^{(k)} \cdot \mathbf{R})}{\omega_h^{(k)} \cdot \mathbf{n}}} \\
\large = \frac{\sum_k^N G(\mathbf{R}, \omega_i^{(k)}) L_i(p, \omega_i) }{\sum_k^N G(\mathbf{R}, \omega_i^{(k)})} \\
$$
根据**Smith shadowing-masking approximation:**
$$
G(\mathbf{n}, \omega_o, \omega_i, k) \approx G(\mathbf{n}, \omega_o, k) G(\mathbf{n}, \omega_i, k) \\
G(\mathbf{n}, \omega, k) = \frac{\mathbf{n} \cdot \omega}{(\mathbf{n} \cdot)(1 - k) + k} \\
$$
**k与粗糙度相关**

因此
$$
G(\mathbf{n}, \omega_i^{(k)}) \approx G(\mathbf{n}, \mathbf{n}, k) G(\mathbf{n}, \omega_i^{(k)}, k) \\
$$
带入$L_c$中，
$$
\large  L_c(R) \approx \frac{\sum_k^N G(\mathbf{R}, \omega_i^{(k)}) L_i(p, \omega_i) }{\sum_k^N G(\mathbf{R}, \omega_i^{(k)})} \\

=\frac{\sum_k^NG(\mathbf{n}, \mathbf{n}, k) G(\mathbf{n}, \omega_i^{(k)}, k) L_i(p, \omega_i) }{\sum_k^N G(\mathbf{n}, \mathbf{n}, k)G(\mathbf{n}, \omega_i^{(k)}, k)} \\

=\frac{\sum_k^N G(\mathbf{n}, \omega_i^{(k)}, k) L_i(p, \omega_i) }{\sum_k^N G(\mathbf{n}, \omega_i^{(k)}, k)} \\

=\frac{\sum_k^N G(\omega_i^{(k)}) L_i(p, \omega_i) }{\sum_k^N G(\omega_i^{(k)})} \\
$$
**对比原来的式子Split Sum approximation**
$$
\large L_s(\omega_o)  &= \int_\Omega f_s L_i(p, \omega_i) \, \mathbf{n} \cdot \omega_i \, d\omega_i \\
&\approx \frac{\int_{\Omega_{f_r}} L_i(p, \omega_i) d\omega_i}{\int_{\Omega_{f_r}} d \omega_i} \ * \ \int_\Omega f_s \mathbf{n} \cdot \omega_i d \omega_i  \\
&\approx (\frac{\sum_k^N L_i(\omega_i^{(k)})W(\omega_i^{(k)})}{\sum_k^N W(\omega_i^{(k)})}) (\frac{1}{N} \sum_j^N \frac{f_s \mathbf{n} \cdot \omega_i^{(j)}}{p(\omega_i^{(k)})})
$$
最终$L_c(R)$与粗糙度和$\omega_i$两个变量相关，在给定粗糙度后，k为定值
$$
W(\omega_i^{(k)}) = \mathbf{n} \cdot \omega_i^{k} \approx \frac{\mathbf{n} \cdot \omega_i^{k}}{(\mathbf{n} \cdot \omega_i^{k})(1- k) +k} = G(\omega_i^{(k)})
$$
**即Split Sum approximation的$W(\omega_i^{(k)})$相当于$G(\omega_i^{(k)})$**



### **Prefilter Map**

计算$L_c(R)=\frac{\sum_k^N G(\omega_i^{(k)}) L_i(p, \omega_i) }{\sum_k^N G(\omega_i^{(k)})} \\$，由于已经做了假设$\omega_o =N, R = N$，忽略了具体的方向，难免会出现误差，**即在掠射角时，无法观察到拖长的反射现象**

![Removing grazing specular reflections with the split sum approximation of V = R = N.](https://learnopengl.com/img/pbr/ibl_grazing_angles.png)

**当前变量只有粗糙度和R，而给定粗糙度时，只剩下一个变量R，因此，可以对$L_c(R)$进行预计算处理**

**使用mipmap存储线性粗糙度0-1，即能够解决粗糙度**，在渲染时，进行插值处理。

**为了加快预计算，同样需要采样重要性采样**.



### BRDF LUT

$$
\large L_s(\omega_o)  
&\approx L_c(R) * \ \int_\Omega f_s \mathbf{n} \cdot \omega_i d \omega_i  \\
$$



右侧积分取决于$\omega_o, \mathbf{n}, F_0和$粗糙度，总共存在三个变量（$\omega_o和\mathbf{n}$夹角$\theta$ ）
$$
\large L_s(\omega_o)  
&\approx L_c(R) * \ \int_\Omega f_s \mathbf{n} \cdot \omega_i d \omega_i  \\
&= \int_\Omega \frac{f_s}{F(\omega_o, h)}(F_0 + (1 - F_0)(1- \omega_o \cdot h)^5) \mathbf{n} \cdot \omega_i d\omega_i \\
&= F_0 \int_\Omega \frac{f_s}{F(\omega_o, h)}(1 -(1- \omega_o \cdot h)^5) \mathbf{n} \cdot \omega_i d\omega_i \\&  + \int_\Omega\frac{f_s}{F(\omega_o, h)}(1- \omega_o \cdot h)^5 \mathbf{n} \cdot \omega_i d\omega_i\\
&=F_0 * scale + bias
$$
积分拆成两项，同时$f_s$中包含F项，可以小曲，因此只剩下$\theta$和粗糙度，同时使用$\cos\theta$作为变量(方便计算[0, 1])。



根据**法线分布函数$p = \frac{D(\omega) (\omega \cdot \mathbf{n})}{4 (\omega_o \cdot  \omega_h)}$**进行重要性采样：
$$
\large \int_\Omega \frac{f_s}{F(\omega_o, h)}(1 -(1- \omega_o \cdot h)^5) \mathbf{n} \cdot \omega_i d\omega_i \\

\large \approx \frac{1}{N} \sum_k^N \frac{D(\omega_h^{(k)})G(\omega_o, \omega_i^{(k)})(1 -(1- \omega_o \cdot h)^5) (\mathbf{n} \cdot \omega_i^{(k)})}{p(\omega_i^{(k)})} \\

\large= \frac{1}{N} \sum_k^N \frac{\frac{D(\omega_h^{(k)})G(\omega_o, \omega_i^{(k)})}{4(\omega_o \cdot \mathbf{n}) (\omega_i^{(k)} \cdot \mathbf{n})}(1 -(1- \omega_o \cdot h)^5) (\mathbf{n} \cdot \omega_i^{(k)})}{\frac{D(\omega_h^{(k)})(\mathbf{n} \cdot \omega_i^{(k)})}{4 (\omega_i^{(k)} \cdot \omega_o)}} \\

\large= \frac{1}{N} \sum_k^N \frac{\frac{G(\omega_o, \omega_i^{(k)})}{(\omega_o \cdot \mathbf{n}) }(1 -(1- \omega_o \cdot h)^5)}{\frac{(\mathbf{n} \cdot \omega_i^{(k)})}{(\omega_i^{(k)} \cdot \omega_o)}} \\

\large= \frac{1}{N} \sum_k^N \frac{G(\omega_o, \omega_i^{(k)})(1 -(1- \omega_o \cdot h)^5) (\omega_i^{(k)} \cdot \omega_o)}{(\mathbf{n} \cdot \omega_i^{(k)}) (\omega_o \cdot \mathbf{n}) } \\
$$

$$
\int_\Omega\frac{f_s}{F(\omega_o, h)}(1- \omega_o \cdot h)^5 \mathbf{n} \cdot \omega_i d\omega_i\\
\large \approx \frac{1}{N} \sum_k^N \frac{D(\omega_h^{(k)})G(\omega_o, \omega_i^{(k)})(1- \omega_o \cdot h)^5 (\mathbf{n} \cdot \omega_i^{(k)})}{p(\omega_i^{(k)})} \\

\large= \frac{1}{N} \sum_k^N \frac{\frac{D(\omega_h^{(k)})G(\omega_o, \omega_i^{(k)})}{4(\omega_o \cdot \mathbf{n}) (\omega_i^{(k)} \cdot \mathbf{n})}(1- \omega_o \cdot h)^5 (\mathbf{n} \cdot \omega_i^{(k)})}{\frac{D(\omega_h^{(k)})(\mathbf{n} \cdot \omega_i^{(k)})}{4 (\omega_i^{(k)} \cdot \omega_o)}} \\

\large= \frac{1}{N} \sum_k^N \frac{\frac{G(\omega_o, \omega_i^{(k)})}{(\omega_o \cdot \mathbf{n}) }(1- \omega_o \cdot h)^5}{\frac{(\mathbf{n} \cdot \omega_i^{(k)})}{(\omega_i^{(k)} \cdot \omega_o)}} \\

\large= \frac{1}{N} \sum_k^N \frac{G(\omega_o, \omega_i^{(k)})(1- \omega_o \cdot h)^5 (\omega_i^{(k)} \cdot \omega_o)}{(\mathbf{n} \cdot \omega_i^{(k)}) (\omega_o \cdot \mathbf{n}) } \\
$$

**使用一个2D纹理，将scale和bias存储在两个通道中，得到BRDF LUT**




$$
\int_0^\pi \cos\theta\sin\theta d\theta
$$
