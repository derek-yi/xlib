## 安装samba
> sudo apt-get install samba

## 创建共享文件夹
假设你要共享的文件夹为：/home/ray/share  
> mkdir /home/ray/share
> chmod 777 /home/ray/share

## 备份并编辑smb.conf
> sudo cp /etc/samba/smb.conf /etc/samba/smb.conf_backup
> sudo gedit /etc/samba/smb.conf

### 将下列几行新增到文件的最后面
> [Share]  
> comment = Shared Folder   
> path = /home/ray/share  
> writable = yes  
> browseable = yes  

###（可选）修改workgroup
找到［global］把 workgroup = MSHOME 改成
workgroup = WORKGROUP

###（可选）在 [global] 放入以下内容
force user = derek
force group = derek
create mask = 0664
directory mask = 0775

## 添加账户和密码
> sudo useradd smbuser
要注意，上面只是增加了smbuser这个用户，却没有给用户赋予本机登录密码。  
所以这个用户将只能从远程访问，不能从本机登录。而且samba的登录密码可以和本机登录密码不一样。

> sudo smbpasswd -a smbuser
设置该账号对应的密码。

> sudo smbpasswd -a derek
也可以把原来的账户直接设置为samba账号


## 重启samba
> sudo /etc/init.d/samba restart
或者：
> sudo service smbd restart

