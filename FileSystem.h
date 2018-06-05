/*
�û�����;Block����
*/

#include<time.h>
#include<string.h>
#include<string>
using namespace std;

//����Ĭ���ļ�ϵͳ������ ���ó���16λ����
const char DEFAULT_SYS_NAME[] = "SysDefault";

//������������
const int COMMAND_NUM = 14;

//����Ĭ���ļ����С�Լ�Ĭ���ļ�������
const unsigned int BLOCK_SIZE = 1024;
const unsigned int BLOCK_NUM = 1024;

//����ṹ
//�ýṹ��Ҫ��SysCommand�������ֵһһ��Ӧ
enum Command
{
	SUDO=0,
	CLS,
	EXIT,
	HELP,
	SYSINFO,
	MKDIR,
	TOUCH,
	LS,
	CD,
	PWD,
	PASSWD,
	RM,
	NANO,
	CAT
};

//�û��ṹ
typedef struct
{
	char username[16];								//�û��������Ƴ���15���ַ� ���һ��Ϊ\0
	char password[16];								//�û����룬���Ƴ���15���ַ�
}User;

//�ļ�ϵͳSuperBlock��ṹ
typedef struct{
    unsigned short blockSize;						//�ļ����С
    unsigned int blockNum;							//�ļ�������
    unsigned int inodeNum;							//i�ڵ�����,���ļ�����
    unsigned int blockFree;							//���п�����
}SuperBlock;


//�ļ�ϵͳInode�ṹ
typedef struct{
    unsigned int id;								//Inode����
    char name[30];									//�ļ�������󳤶�29
    unsigned char isDir;							//�ļ����� 0-file 1-dir
    unsigned int parentID;							//��Ŀ¼inode����
    unsigned int length;							//�ļ�����,���ΪBLOCK_SIZE/sizeof(Fcb)
    time_t time;									//�ļ�����޸�ʱ�䣬��1970��1��1��00ʱ00��00������������������
    unsigned int blockId;							//�ļ������ڵ�Ŀ¼���ݿ��id������ɾ��ʱ��λ
}Inode;

//�ļ��Ĳ�����Ϣ
typedef struct 
{
	unsigned int id;
	char name[30];
	unsigned char isDir;
	unsigned int blockId;
}Fcb;

class FileSystem
{
public:
	FileSystem(char* name);							//����ϵͳ�������� Ĭ����Ϣ
	virtual ~FileSystem();

	//��ʼ����Ϣ
	int InitSystem();								//ϵͳ��ʼ��
	void OpenFileSystem();							//���ļ�ϵͳ
	void CreateFileSystem();						//�����ļ�ϵͳ

	int findInodeID(char *name,int flag);			//���ļ���Ŀ¼

	//user�������
	void SetUser(User user);						//�����û�	
	void GetUser(User* puser);						//��ȡ֮ǰ���õ��û���Ϣ

	//SuperBlock�������
	void SetSuperBlock(SuperBlock block);			//����SuperBlock������Ϣ
	void GetSuperBlock(SuperBlock* pblock);			//��ȡ�ļ�ϵͳ��SuperBlock��Ϣ

	//BlockBitmap�������
	void SetBlockBitmap(unsigned char* bitmap,unsigned int start,unsigned int count);
	void GetBlockBitmap(unsigned char* bitmap);

	//InodeBitmap�������
    void SetInodeBitmap(unsigned char* bitmap, unsigned int start, unsigned int count);
	void GetInodeBitmap(unsigned char* bitmap);

	//Inode�������
	void GetInode(Inode *pInode, unsigned int id);
	void SetInode(Inode inode);

	void login();									//��¼
	void SetLoginTime();							//���õ�¼ʱ��
	void GetLoginTime();							//��ȡ����¼ʱ��

	//�û�����
	void clear();									//���
	void exitSystem();								//�˳�����
	void help();									//ϵͳ�İ����ĵ�
	void systemInfo();								//�鿴�ļ�ϵͳ��Ϣ
	int createFile(char* name,unsigned short isDir);//�����ļ���Ŀ¼
	void ls(char* arg);								//��ʾlist
	void cd(char* name);							//������Ŀ¼
	void pwd();										//��ʾ��ǰ·��
	void passwd();									//�޸�����
	void rm(char* name);							//ɾ���ļ�
	void nano(char* name);							//д�����ֵ��ļ�
	void cat(char *name);							//��ȡ�ļ��������
	
	void command();									//�ȴ�����
	void Init_SysCommand();							//����ϵͳ����
	int analysis(string str);						//��������
	void showpath();								//��ʾ��ǰ·��

private:
	char system_name[17];							//�ļ�ϵͳ����;���Ƴ���16λ
	FILE *fp;										//�ļ�ϵͳ�ļ�ָ��
	string current_path;							//��ǰϵͳ·��
	SuperBlock SuperBlock_Info;						//�ļ�ϵͳ����Ϣ
	User user;										//�û�
	Inode current_Inode;							//��ǰ��Ŀ¼i�ڵ�
	
	string SysCommand[COMMAND_NUM];					//ϵͳ֧�ֵ�����;�ܸ���n��
	char cmd[5][10];								//�������������5��������ÿ���������9���ַ�

	char LastLoginTime[64];							//����¼ʱ��
	//��������ǵ�ʱ�䣬���԰�����ƫ�Ʒŵ�����󣬲��Ķ�ǰ���ƫ��

	unsigned char BlockBitmap[BLOCK_NUM];			//block���ձ�
	unsigned char InodeBitmap[BLOCK_NUM];			//inode���ձ�
	//�ļ�ϵͳ���������С
	unsigned short UserSize;						//�û�����Ĵ�С
	unsigned short SuperBlockSize;					//SuperBlock������Ĵ�С
	unsigned int BlockBitmapSize;					//block���ձ�Ĵ�С
	unsigned int InodeBitmapSize;					//inode���ձ�Ĵ�С
	unsigned int InodeSize;							//Inode�����С
	unsigned int BlockSize;							//Block�����С
	unsigned int LoginSize;							//��¼ʱ��Ĵ�С

	//��������֮����뿪ͷ��ƫ����
	unsigned long SuperBlockOffset;					//SuperBlock���ƫ����
	unsigned long BlockBitmapOffset;				//block���ձ��ƫ����
	unsigned long InodeBitmapOffset;				//inode���ձ��ƫ����
	unsigned long InodeOffset;						//Inode�����ƫ��
	unsigned long BlockOffset;						//Block�����ƫ��
	unsigned long LoginOffset;						//��¼ʱ��ƫ��
};

