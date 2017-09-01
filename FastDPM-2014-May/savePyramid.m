function savePyramid(pyra)
% ��featpyramid()�������������pyramid������txt
% ���о���Ԫ�ذ���openCV���������з�ʽ�洢

clc
fid = fopen('pyramid.txt','w');

fprintf(fid,'imsize\t%d\t%d',pyra.imsize(1),pyra.imsize(2));
fprintf(fid,'\nnum_levels\t%d',pyra.num_levels);
fprintf(fid,'\npadx\t%d',pyra.padx);
fprintf(fid,'\npady\t%d',pyra.pady);

fprintf(fid,'\nscales %d',length(pyra.scales));
for i=1:length(pyra.scales)
    fprintf(fid,' %g',pyra.scales(i));
end

fprintf(fid,'\nvalid_levels %d',length(pyra.valid_levels));
for i=1:length(pyra.valid_levels)
    fprintf(fid,' %g',pyra.valid_levels(i));
end

fprintf(fid,'\nfeat %d',length(pyra.feat));
for i=1:length(pyra.feat)
    F = pyra.feat{i};
    fprintf(fid,'\nfeat{%d} %d %d %d\n',i,size(F,1),size(F,2),size(F,3));
    % ������F��Ԫ����openCV�����ݴ洢��ʽ����
    F2 = zeros(1,numel(F));
    idx = 1;
    for r=1:size(F,1)
        for c=1:size(F,2)
            for d=1:size(F,3)
                F2(idx) = F(r,c,d);
                idx = idx + 1;                
            end
        end
    end
    fprintf(fid,' %g',F2);
    fprintf(fid,' END');
end

fclose(fid);

