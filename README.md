# 基于C++17的简易线程池

> 所学知识参考链接：
>
> C++11 thread， C++11 mutex：https://wyqz.top/p/2668140628.html
>
> C++11 左右值与移动构造函数：http://avdancedu.com/a39d51f9/



- 实现多线程安全的任务队列，线程池使用异步操作，提交(submit)使用与thread相同。

- 内部利用完美转发获取可调用对象的函数签名，lambda与function包装任务，使用RAII管理线程池的生命周期。

# std::move

将`左值或右值`转换为右值

# 完美转发

> 参考：http://c.biancheng.net/view/7868.html

指的是函数模板可以将自己的参数“完美”地转发给内部调用的其它函数。所谓完美，即不仅能准确地转发参数的值，还能保证被转发参数的左、右值属性不变。

C++11 标准中规定，通常情况下右值引用形式的参数只能接收右值，不能接收左值。但对于函数模板中使用右值引用语法定义的参数来说，它不再遵守这一规定，既可以接收右值，也可以接收左值，（此时的右值引用又被称为“万能引用”）。

通过将函数模板的形参类型设置为 T&&，我们可以很好地解决接收左、右值的问题。

C++11 标准的开发者已经帮我们想好了解决方案，该新标准还引入了一个模板函数 `forword<T>()`

# RAII

> 参考：https://zhuanlan.zhihu.com/p/600337719

**RAII**，全称**资源获取即初始化**，C++语言的一种管理资源、避免泄漏的惯用法

> RAII要求，资源的有效期与持有资源的[对象的生命期](https://link.zhihu.com/?target=https%3A//zh.wikipedia.org/w/index.php%3Ftitle%3D%E5%AF%B9%E8%B1%A1%E7%9A%84%E7%94%9F%E5%91%BD%E6%9C%9F%26action%3Dedit%26redlink%3D1)严格绑定，即由对象的[构造函数](https://link.zhihu.com/?target=https%3A//zh.wikipedia.org/wiki/%E6%9E%84%E9%80%A0%E5%87%BD%E6%95%B0)完成[资源的分配](https://link.zhihu.com/?target=https%3A//zh.wikipedia.org/w/index.php%3Ftitle%3D%E8%B5%84%E6%BA%90%E7%9A%84%E5%88%86%E9%85%8D%26action%3Dedit%26redlink%3D1)（获取），同时由[析构函数](https://link.zhihu.com/?target=https%3A//zh.wikipedia.org/wiki/%E6%9E%90%E6%9E%84%E5%87%BD%E6%95%B0)完成资源的释放。在这种要求
>
> 下，只要对象能正确地析构，就不会出现[资源泄漏](https://link.zhihu.com/?target=https%3A//zh.wikipedia.org/wiki/%E8%B5%84%E6%BA%90%E6%B3%84%E6%BC%8F)问题。

当我们在一个函数内部使用局部变量，当退出了这个局部变量的作用域时，这个变量也就别销毁了；

当这个变量是类对象时，这个时候，就会自动调用这个类的析 构函数，而这一切都是自动发生的，不要程序员显示的去调用完成。RAII就是这样去完成的。

**由于系统的资源不具有自动释放的功能**，而C++中的类具有自动调用析构函数的功能。如果把资源用类进行封装起来，对资源操作都封装在类的内部，在析构函数中进行释放资源。**当定义的局部变量的生命结束时，它的析构函数就会自动的被调用**，如此，就不用程序员显示的去调用释放资源的操作了。
